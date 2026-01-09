export type RangeKey = '1h' | '1d' | '30d';
export type BucketKey = 'raw' | 'hour' | 'day';

export interface DataPoint {
  t: number;
  v: number;
}

export interface ApiCurrent {
  epoch_ms?: number;
  value?: number;
  time?: number;
}

export interface ApiStats {
  bucket?: string;
  data?: Array<[number, number] | Record<string, unknown>>;
  samples?: Array<[number, number] | Record<string, unknown>>;
  rows?: Array<[number, number] | Record<string, unknown>>;
}
