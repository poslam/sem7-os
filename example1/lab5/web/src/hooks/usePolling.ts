import { useEffect, useState } from 'react';

export function usePolling<T>(
  fetcher: () => Promise<T>,
  deps: unknown[] = [],
  interval = 1000,
) {
  const [data, setData] = useState<T | null>(null);

  useEffect(() => {
    let active = true;

    const tick = async () => {
      try {
        const res = await fetcher();
        if (!active) return;
        setData(res);
      } catch (err) {
        console.error(err);
      }
    };

    tick();
    const id = setInterval(tick, interval);
    return () => {
      active = false;
      clearInterval(id);
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, deps);

  return data;
}
