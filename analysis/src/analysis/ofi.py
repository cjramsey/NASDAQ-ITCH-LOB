"""Shared pipeline for the Cont, Kukanov & Stoikov (2014) OFI analyses.

Consumes the BBO event stream exported to out/depth.parquet (one row per
book-mutating message; level 00 columns are the best quotes). Used by
ofi_analysis.ipynb and depth_scaling.ipynb.
"""

from collections.abc import Iterator, Sequence
from pathlib import Path

import polars as pl
import statsmodels.api as sm

OUT_DIR = Path(__file__).resolve().parents[3] / "out"

TICK = 100                                           # $0.01 in 1e-4 dollar units
RTH_START, RTH_END = 34_200 * 10**9, 57_600 * 10**9  # 09:30, 16:00 in ns since midnight
DT_NS = 10 * 10**9                                   # 10s intervals
HALF_HOUR_NS = 1_800 * 10**9


def depth_columns(levels: int = 1) -> list[str]:
    """Depth column names as exported by depth_writer.cpp ("%s_%02zu" naming)."""
    return [
        f"{side}_{kind}_{i:02d}"
        for i in range(levels)
        for side in ("bid", "ask")
        for kind in ("px", "sz")
    ]


def load_bbo(
    tickers: str | Sequence[str], levels: int = 1
) -> pl.DataFrame | dict[str, pl.DataFrame]:
    """Regular-hours depth event stream, both sides quoted at the best level.

    A single ticker returns one DataFrame; a sequence returns {ticker: DataFrame}. 
    Rows where the book is shallower than `levels` keep nulls in the deeper columns; 
    only a one-sided best level drops the row.
    """
    single = isinstance(tickers, str)
    wanted = [tickers] if single else list(tickers)
    lf = (
        pl.scan_parquet(OUT_DIR / "depth.parquet")
        .filter(pl.col("stock").is_in(wanted))
        .filter(pl.col("timestamp_ns").is_between(RTH_START, RTH_END, closed="left"))
        .drop_nulls(["bid_px_00", "ask_px_00"])
        .select("timestamp_ns", "stock", *depth_columns(levels))
    )
    df = lf.collect(engine="streaming")
    if single:
        return df.drop("stock")
    return {k[0]: v.drop("stock") for k, v in df.partition_by("stock", as_dict=True).items()}


def iter_bbo(
    tickers: Sequence[str], levels: int = 1, chunk: int = 5
) -> Iterator[tuple[str, pl.DataFrame]]:
    """Yield (ticker, frame) pairs, scanning depth.parquet once per `chunk` tickers."""
    tickers = list(tickers)
    for start in range(0, len(tickers), chunk):
        group = load_bbo(tickers[start : start + chunk], levels)
        for t in tickers[start : start + chunk]:
            if t in group:
                yield t, group.pop(t)


def event_order_flow(bbo: pl.DataFrame) -> pl.DataFrame:
    """Per-event OFI contribution e_n (paper eq. 10) and the mid-price after the event.

    e_n =  1{Pb_n >= Pb_{n-1}} qb_n - 1{Pb_n <= Pb_{n-1}} qb_{n-1}
         - 1{Pa_n <= Pa_{n-1}} qa_n + 1{Pa_n >= Pa_{n-1}} qa_{n-1}
    """
    pb, qb = pl.col("bid_px_00"), pl.col("bid_sz_00").cast(pl.Int64)
    pa, qa = pl.col("ask_px_00"), pl.col("ask_sz_00").cast(pl.Int64)
    e_n = (
        (pb >= pb.shift(1)).cast(pl.Int64) * qb
        - (pb <= pb.shift(1)).cast(pl.Int64) * qb.shift(1)
        - (pa <= pa.shift(1)).cast(pl.Int64) * qa
        + (pa >= pa.shift(1)).cast(pl.Int64) * qa.shift(1)
    )
    mid = (pb + pa) / 2
    return bbo.with_columns(e_n.alias("e_n"), mid.alias("mid")).slice(1)  # row 0 has no predecessor


def ofi_intervals(events: pl.DataFrame, dt_ns: int = DT_NS) -> pl.DataFrame:
    """Aggregate events to dt_ns intervals: OFI_k = sum of e_n, dP = mid change in ticks."""
    return (
        events.group_by((pl.col("timestamp_ns") // dt_ns).alias("bucket"), maintain_order=True)
        .agg(pl.col("e_n").sum().alias("ofi"), pl.col("mid").last().alias("mid_close"))
        .with_columns(((pl.col("mid_close") - pl.col("mid_close").shift(1)) / TICK).alias("dP"))
        .drop_nulls("dP")
    )


def fit_price_impact(iv: pl.DataFrame):
    """OLS dP_k = alpha + beta * OFI_k + eps, White (HC1) standard errors as in the paper."""
    return sm.OLS(
        iv["dP"].to_numpy(), sm.add_constant(iv["ofi"].to_numpy().astype(float))
    ).fit(cov_type="HC1")


def window_fits(
    events: pl.DataFrame, dt_ns: int = DT_NS, window_ns: int = HALF_HOUR_NS
) -> pl.DataFrame:
    """Per-window price-impact fits with the window's average depth at the best quotes.

    Returns one row per window: window (timestamp_ns // window_ns), n_intervals,
    beta, se, t_beta, avg_depth (mean of (bid_sz_00 + ask_sz_00) / 2).
    """
    iv = ofi_intervals(events, dt_ns).with_columns(
        (pl.col("bucket") * dt_ns // window_ns).alias("window")
    )
    depth = events.group_by((pl.col("timestamp_ns") // window_ns).alias("window")).agg(
        ((pl.col("bid_sz_00") + pl.col("ask_sz_00")) / 2).mean().alias("avg_depth")
    )
    rows = []
    for (w,), g in sorted(iv.group_by("window"), key=lambda kv: kv[0][0]):
        f = fit_price_impact(g)
        rows.append(
            {
                "window": int(w),
                "n_intervals": len(g),
                "beta": f.params[1],
                "se": f.bse[1],
                "t_beta": f.tvalues[1],
            }
        )
    return pl.DataFrame(rows).join(depth, on="window", how="left").sort("window")
