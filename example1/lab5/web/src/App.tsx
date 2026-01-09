import React, { useMemo, useState } from 'react';
import { usePolling } from './hooks/usePolling';
import { ApiCurrent, ApiStats, BucketKey, DataPoint, RangeKey } from './types';
import LineChart from './components/LineChart';
import DataTable from './components/DataTable';

const apiBase = '';

const durations: Record<RangeKey, number> = {
  '1h': 60 * 60 * 1000,
  '1d': 24 * 60 * 60 * 1000,
  '30d': 30 * 24 * 60 * 60 * 1000,
};

const rangeLabels: Record<RangeKey, string> = {
  '1h': 'Последний час',
  '1d': 'Последние сутки',
  '30d': 'Последние 30 дней',
};

const bucketOptions: { value: BucketKey; label: string }[] = [
  { value: 'raw', label: 'Сырые измерения' },
  { value: 'hour', label: 'Почасовой средний' },
  { value: 'day', label: 'Дневной средний' },
];

const formatDate = (ts: number) => new Date(ts).toLocaleString('ru-RU');

const normalizeSeries = (stats: ApiStats | null): DataPoint[] => {
  const rows = stats?.data || stats?.samples || stats?.rows || [];
  if (!rows || !Array.isArray(rows)) return [];

  return rows
    .map((s) => {
      if (Array.isArray(s)) return { t: Number(s[0]), v: Number(s[1]) };
      const record = s as Record<string, unknown>;
      return {
        t: Number(
          record.epoch_ms ??
            record.time ??
            record.ts ??
            record.timestamp ??
            0,
        ),
        v: Number(record.value ?? record.val ?? record.temperature ?? 0),
      };
    })
    .filter((s) => s.t && Number.isFinite(s.v));
};

function App() {
  const [bucket, setBucket] = useState<BucketKey>('raw');
  const [range, setRange] = useState<RangeKey>('1h');

  const current = usePolling<ApiCurrent | null>(async () => {
    const res = await fetch(`${apiBase}/api/current`);
    if (!res.ok) throw new Error('current fetch failed');
    return res.json();
  }, [], 1000);

  const stats = usePolling<ApiStats | null>(async () => {
    const now = Date.now();
    const start = now - durations[range];
    const bucketParam =
      bucket === 'hour' ? 'hourly' : bucket === 'day' ? 'daily' : 'raw';
    const url = `${apiBase}/api/stats?bucket=${bucketParam}&start=${start}&end=${now}`;
    const res = await fetch(url);
    if (!res.ok) throw new Error('stats fetch failed');
    return res.json();
  }, [bucket, range], 1000);

  const series = useMemo(() => normalizeSeries(stats), [stats]);
  const tableRows = useMemo(() => [...series].sort((a, b) => b.t - a.t), [series]);

  const currentText =
    current && current.value != null
      ? `${current.value.toFixed(2)} °C • ${formatDate(
          current.epoch_ms || current.time || Date.now(),
        )}`
      : 'Загрузка...';

  return (
    <div className="space-y-4">
      <header className="flex flex-col gap-2">
        <h1 className="text-3xl font-bold text-slate-50">Панель температуры</h1>
      </header>

      <section className="rounded-2xl border border-slate-800 bg-slate-900/60 p-4 shadow-lg shadow-brand-500/10">
        <div className="flex items-center justify-between gap-4 flex-wrap">
          <div>
            <p className="text-sm text-slate-400">Текущее значение</p>
            <p className="text-xl font-semibold text-brand-100">{currentText}</p>
          </div>
          <div className="flex items-center gap-3">
            <label className="text-sm text-slate-300">Агрегация</label>
            <select
              className="bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm"
              value={bucket}
              onChange={(e) => setBucket(e.target.value as BucketKey)}
            >
              {bucketOptions.map((opt) => (
                <option key={opt.value} value={opt.value}>
                  {opt.label}
                </option>
              ))}
            </select>
          </div>
        </div>
      </section>

      <section className="rounded-2xl border border-slate-800 bg-slate-900/60 p-4 shadow-lg shadow-brand-500/10 space-y-4">
        <div className="flex items-center gap-3 flex-wrap">
          <p className="text-sm text-slate-300">Диапазон:</p>
          {Object.keys(rangeLabels).map((key) => (
            <button
              key={key}
              onClick={() => setRange(key as RangeKey)}
              className={`px-4 py-2 rounded-lg text-sm font-semibold border transition ${
                range === key
                  ? 'bg-brand-500 text-slate-900 border-brand-500'
                  : 'bg-slate-800 border-slate-700 text-slate-200 hover:border-brand-400/60'
              }`}
            >
              {rangeLabels[key as RangeKey]}
            </button>
          ))}
        </div>

        <div className="space-y-3">
          <div className="flex items-center justify-between text-sm text-slate-300">
            <span className="font-semibold">График</span>
            <span className="text-slate-500">Обновляется каждую секунду</span>
          </div>
          <LineChart data={series} formatDate={formatDate} />
        </div>

        <div className="space-y-3">
          <div className="flex items-center justify-between text-sm text-slate-300">
            <span className="font-semibold">Таблица</span>
            <span className="text-slate-500">Свежие сверху • автообновление</span>
          </div>
          <DataTable rows={tableRows} formatDate={formatDate} />
        </div>
      </section>
    </div>
  );
}

export default App;
