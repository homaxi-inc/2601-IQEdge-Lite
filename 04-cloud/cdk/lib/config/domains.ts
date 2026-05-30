/**
 * Five G2 domains — canonical list for CDK (must match G2_Domain_Map.md).
 */
export const G2_DOMAINS = [
  'energy',
  'network',
  'vision',
  'environment',
  'control',
] as const;

export type G2Domain = (typeof G2_DOMAINS)[number];

/** MQTT topic suffix per domain (uplink unless noted). */
export const G2_DOMAIN_MQTT_SUFFIXES: Record<G2Domain, readonly string[]> = {
  energy: ['telemetry'],
  network: ['telemetry'],
  vision: ['event', 'telemetry'],
  environment: ['telemetry'],
  control: ['command'],
};
