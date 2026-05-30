import type { G2Domain } from './domains';

/** Timestream table suffix per domain (matches G2_Domain_Map.md). */
export const G2_TIMESTREAM_TABLE_SUFFIX: Record<G2Domain, string> = {
  energy: 'table_energy',
  network: 'table_network',
  vision: 'table_vision',
  environment: 'table_environment',
  control: 'table_control_logs',
};
