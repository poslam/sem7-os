import React from 'react';
import { DataPoint } from '../types';

interface Props {
  rows: DataPoint[];
  formatDate: (ts: number) => string;
}

export const DataTable: React.FC<Props> = ({ rows, formatDate }) => {
  if (!rows || rows.length === 0) {
    return <div className="text-slate-400">Нет данных</div>;
  }
  return (
    <div className="overflow-hidden rounded-2xl border border-slate-800 bg-slate-900/60">
      <div className="max-h-72 overflow-y-auto">
        <table className="min-w-full text-sm">
          <thead className="bg-slate-800/60 text-slate-200">
            <tr>
              <th className="px-4 py-2 text-left font-semibold">Время</th>
              <th className="px-4 py-2 text-right font-semibold">Значение</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800 text-slate-100">
            {rows.map((r, idx) => (
              <tr key={idx} className="hover:bg-slate-800/40">
                <td className="px-4 py-2 whitespace-nowrap">{formatDate(r.t)}</td>
                <td className="px-4 py-2 text-right font-semibold">{(r.v ?? 0).toFixed(2)} °C</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
};

export default DataTable;
