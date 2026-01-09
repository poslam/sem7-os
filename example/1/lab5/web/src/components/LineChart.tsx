import React, { useEffect, useMemo, useRef, useState } from 'react';
import { DataPoint } from '../types';

const clamp = (n: number, min: number, max: number) => Math.max(min, Math.min(max, n));

type Point = DataPoint & { x: number; y: number };
type HoverState = (Point & { left: number; top: number }) | null;

interface Props {
  data: DataPoint[];
  height?: number;
  formatDate: (ts: number) => string;
}

export const LineChart: React.FC<Props> = ({ data, height = 260, formatDate }) => {
  const paddingX = 12;
  const paddingY = 16;
  const containerRef = useRef<HTMLDivElement | null>(null);
  const [size, setSize] = useState({ width: 1200, height });
  const chartWidth = size.width;
  const chartHeight = size.height;
  const [hover, setHover] = useState<HoverState>(null);
  const [zoom, setZoom] = useState(1); // 1 = весь диапазон
  const [viewEnd, setViewEnd] = useState<number | null>(null); // null = следуем за последними точками
  const [dragging, setDragging] = useState(false);
  const dragStart = useRef<{ x: number; end: number }>({ x: 0, end: 0 });

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;
    const update = () => {
      const w = Math.max(el.clientWidth, 320);
      setSize({ width: w, height });
    };
    update();
    const ro = new ResizeObserver(update);
    ro.observe(el);
    return () => ro.disconnect();
  }, [height]);

  const { points, minT, maxT, viewSpan } = useMemo(() => {
    if (!data || data.length === 0) {
      return { points: [] as Point[], minT: 0, maxT: 0, viewSpan: 1 };
    }

    let series = [...data].sort((a, b) => a.t - b.t);
    const times = series.map((s) => s.t);
    const vals = series.map((s) => s.v);
    const minTVal = Math.min(...times);
    const maxTVal = Math.max(...times);
    const minV = Math.min(...vals);
    const maxV = Math.max(...vals);
    const spanT = maxTVal - minTVal || 1;
    const spanV = maxV - minV || 1;

    const desiredSpan = spanT / zoom;
    const endCandidate = viewEnd ?? maxTVal;
    const clampedEnd = clamp(endCandidate, minTVal + desiredSpan, maxTVal);
    const windowStart = clampedEnd - desiredSpan;

    series = series.filter((s) => s.t >= windowStart && s.t <= clampedEnd);

    const visMinT = Math.min(...series.map((s) => s.t));
    const visMaxT = Math.max(...series.map((s) => s.t));
    const visSpanT = visMaxT - visMinT || 1;

    const pts = series.map((s) => ({
      x: paddingX + ((s.t - visMinT) / visSpanT) * (chartWidth - paddingX * 2),
      y: chartHeight - paddingY - ((s.v - minV) / spanV) * (chartHeight - paddingY * 2),
      ...s,
    }));

    return { points: pts, minT: minTVal, maxT: maxTVal, viewSpan: desiredSpan };
  }, [chartHeight, chartWidth, data, viewEnd, zoom]);

  if (!points.length) {
    return <div className="h-[260px] flex items-center justify-center text-slate-400">Нет данных</div>;
  }

  const pathD = points.map((p, i) => `${i === 0 ? 'M' : 'L'} ${p.x} ${p.y}`).join(' ');

  const handleMove = (e: React.MouseEvent<SVGSVGElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const scaleX = chartWidth / rect.width;
    const x = (e.clientX - rect.left) * scaleX;

    if (dragging) {
      const deltaX = e.clientX - dragStart.current.x;
      const msPerPx = viewSpan / Math.max(1, chartWidth - paddingX * 2);
      const nextEnd = dragStart.current.end - deltaX * msPerPx;
      const clamped = clamp(nextEnd, minT + viewSpan, maxT);
      setViewEnd(clamped);
      setHover(null);
      return;
    }

    const best = points.reduce<Point | null>((acc, p) => {
      const d = Math.abs(p.x - x);
      if (acc === null || d < Math.abs(acc.x - x)) return p;
      return acc;
    }, null);
    if (!best) return;
    const left = clamp(best.x + 10, paddingX + 70, chartWidth - paddingX - 120);
    const top = clamp(best.y - 60, 8, chartHeight - 72);
    setHover({ ...best, left, top });
  };

  const handleDown = (e: React.MouseEvent<SVGSVGElement>) => {
    setDragging(true);
    dragStart.current = { x: e.clientX, end: viewEnd ?? maxT };
    setHover(null);
  };

  const handleUp = () => {
    setDragging(false);
  };

  const handleLeave = () => {
    setDragging(false);
    setHover(null);
  };

  const zoomIn = () => setZoom((z) => clamp(z * 1.5, 1, 50));
  const zoomOut = () => setZoom((z) => Math.max(1, z / 1.5));
  const zoomReset = () => {
    setZoom(1);
    setViewEnd(null);
    setHover(null);
  };
  const followLatest = () => {
    setViewEnd(null);
    setHover(null);
  };

  return (
    <div className="relative w-full" ref={containerRef}>
      <div className="absolute right-2 top-2 z-10 flex items-center gap-1 text-xs text-slate-200">
        <button
          onClick={followLatest}
          className={`px-2 py-1 rounded-md border ${viewEnd === null ? 'bg-brand-500 text-slate-900 border-brand-500' : 'bg-slate-800/80 border-slate-700 hover:border-brand-400/60'}`}
        >
          Live
        </button>
        <button
          onClick={zoomOut}
          className="px-2 py-1 rounded-md bg-slate-800/80 border border-slate-700 hover:border-brand-400/60"
        >
          −
        </button>
        <button
          onClick={zoomIn}
          className="px-2 py-1 rounded-md bg-slate-800/80 border border-slate-700 hover:border-brand-400/60"
        >
          +
        </button>
        <button
          onClick={zoomReset}
          className="px-2 py-1 rounded-md bg-slate-800/80 border border-slate-700 hover:border-brand-400/60"
        >
          1:1
        </button>
        <span className="px-2 py-1 rounded-md bg-slate-900/80 border border-slate-800">x{zoom.toFixed(1)}</span>
      </div>
      <svg
        viewBox={`0 0 ${chartWidth} ${chartHeight}`}
        className="w-full bg-slate-900/60 rounded-2xl border border-slate-800"
        style={{ height: chartHeight }}
        onMouseMove={handleMove}
        onMouseLeave={handleLeave}
        onMouseDown={handleDown}
        onMouseUp={handleUp}
      >
        <defs>
          <linearGradient id="lineFill" x1="0" x2="0" y1="0" y2="1">
            <stop offset="0%" stopColor="#22d3ee" stopOpacity="0.28" />
            <stop offset="100%" stopColor="#22d3ee" stopOpacity="0" />
          </linearGradient>
        </defs>
        <path d={pathD} fill="none" stroke="#22d3ee" strokeWidth="2.4" />
        <path
          d={`${pathD} L ${points[points.length - 1].x} ${chartHeight - paddingY} L ${points[0].x} ${
            chartHeight - paddingY
          } Z`}
          fill="url(#lineFill)"
          stroke="none"
        />
        {points.map((p, i) => (
          <circle key={i} cx={p.x} cy={p.y} r="3.2" fill="#22d3ee" opacity="0.85" />
        ))}
        {hover && <circle cx={hover.x} cy={hover.y} r="6" fill="#f97316" stroke="#0f172a" strokeWidth="2" />}
      </svg>
      {hover && (
        <div
          className="absolute px-3 py-2 text-xs bg-slate-900/90 border border-amber-400/60 text-slate-100 rounded-xl shadow-xl pointer-events-none"
          style={{ left: hover.left, top: hover.top }}
        >
          <div className="font-semibold">{hover.v.toFixed(2)} °C</div>
          <div className="text-slate-400">{formatDate(hover.t)}</div>
        </div>
      )}
    </div>
  );
};

export default LineChart;
