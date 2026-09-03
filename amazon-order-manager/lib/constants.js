/* Orderkoll – delade konstanter.
 * Laddas som klassiskt skript i content scripts, popup, options och (via
 * importScripts) i service workern. Allt hängs på globalen AOM. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  /** Normaliserade artikelstatusar (se arbetsorder 5.1). */
  const STATUS = {
    NOT_SHIPPED: 'ej_skickad',
    ON_WAY: 'pa_vag',
    DELIVERED: 'levererad',
    CANCELLED: 'avbruten',
  };

  const STATUS_LABEL = {
    ej_skickad: 'Ej skickad',
    pa_vag: 'På väg',
    levererad: 'Levererad',
    avbruten: 'Avbruten',
  };

  /** Meddelandetyper mellan content scripts, popup/options och service worker. */
  const MSG = {
    // content script -> service worker
    ORDER_HISTORY_PARSED: 'orderHistoryParsed',
    ORDER_DETAIL_PARSED: 'orderDetailParsed',
    RETURNS_PARSED: 'returnsParsed',
    PRODUCT_SNAPSHOT: 'productSnapshot',
    PROVISIONAL_SNAPSHOT: 'provisionalSnapshot',
    // ui -> service worker
    GET_STATE: 'getState',
    SYNC_NOW: 'syncNow',
    SET_RECEIVED: 'setReceived',
    SET_RETURN_REQUESTED: 'setReturnRequested',
    SET_HAS_DEFECT: 'setHasDefect',
    SETTINGS_UPDATE: 'settingsUpdate',
    WATCH_ADD: 'watchAdd',
    WATCH_REMOVE: 'watchRemove',
    PROOF_LIST: 'proofList',
    PROOF_GET: 'proofGet',
    PROOF_DELETE: 'proofDelete',
    PROOF_CAPTURE_FOR_ORDER: 'proofCaptureForOrder',
    FETCH_URL: 'fetchUrl',
    IMAGE_HASH: 'imageHash',
    FX_RATES: 'fxRates',
    STORAGE_USAGE: 'storageUsage',
    CLEAR_ALL: 'clearAll',
    EXPORT_ALL: 'exportAll',
    // service worker -> ui/tab
    STATE_CHANGED: 'stateChanged',
    TOGGLE_OVERLAY: 'toggleOverlay',
    OPEN_URL: 'openUrl',
    RESCAN: 'rescan',
  };

  const STORAGE_KEYS = {
    ORDERS: 'orders',
    SELLERS: 'sellers',
    SETTINGS: 'settings',
    SYNC_STATE: 'syncState',
    WATCHES: 'watches',
    PROOF_INDEX: 'proofIndex',
    REFUND_RECORDS: 'refundRecords',
    PARSE_REPORTS: 'parseReports',
    FX: 'fxRates',
  };

  const ALARM_SYNC = 'aom-order-sync';

  /** Amazon-marknader som kan ingå i prisjämförelsen (arbetsorder 9.3). */
  const MARKETPLACES = [
    { id: 'se', domain: 'www.amazon.se', currency: 'SEK', label: 'Amazon.se' },
    { id: 'de', domain: 'www.amazon.de', currency: 'EUR', label: 'Amazon.de' },
    { id: 'fr', domain: 'www.amazon.fr', currency: 'EUR', label: 'Amazon.fr' },
    { id: 'it', domain: 'www.amazon.it', currency: 'EUR', label: 'Amazon.it' },
    { id: 'es', domain: 'www.amazon.es', currency: 'EUR', label: 'Amazon.es' },
    { id: 'nl', domain: 'www.amazon.nl', currency: 'EUR', label: 'Amazon.nl' },
    { id: 'uk', domain: 'www.amazon.co.uk', currency: 'GBP', label: 'Amazon.co.uk' },
  ];

  const DEFAULT_SETTINGS = {
    /** Bakgrundssynk. 30 min är förslaget i arbetsordern (9.1) och kan ändras. */
    backgroundSyncEnabled: true,
    syncIntervalMinutes: 30,
    /** Chrome har ingen helt osynlig flik – se README. */
    backgroundTabMode: 'background-tab', // 'background-tab' | 'minimized-window'
    /** Max antal orderdetaljsidor som besöks per synkomgång (skonsamt mot Amazon). */
    maxDetailVisitsPerSync: 4,
    /** Slumpad paus mellan varje sidhämtning mot Amazon, millisekunder. */
    requestDelayMinMs: 3000,
    requestDelayMaxMs: 9000,
    notificationsEnabled: true,
    /** Prisjämförelse. */
    compareMarketplaces: ['de', 'fr', 'it', 'es', 'nl', 'uk'],
    imageMatchThreshold: 0.85,
    /** Lagerbevakning: minsta intervall per produkt. */
    stockCheckMinIntervalMinutes: 60,
    /** Bevisarkiv. null = nedskalning avstängd (öppet beslut 9.4). */
    proofDownscaleAfterDays: null,
    provisionalTtlDays: 4,
    /** Bank-/kortreklamationsfrister. Inget standardvärde (öppet beslut 9.2). */
    bankDisputeDeadlines: [], // [{ label: 'Visa ••4417', days: 120 }]
    /** Sätts till true när användaren kvitterat risk- och policytexten. */
    riskNoticeAcknowledged: false,
  };

  const DEFAULT_SYNC_STATE = {
    lastSyncAt: null,
    lastSyncSource: null,
    lastError: null,
    blockedReason: null, // 'captcha' | 'signin' | 'http' | null
    running: false,
  };

  const DISCLAIMER =
    'Detta tillägg är oberoende och inte skapat, godkänt av eller anslutet till Amazon.';

  const RISK_NOTICE =
    'Tillägget läser ditt eget Amazon-konto via din egen inloggade session. ' +
    'Amazons användarvillkor förbjuder automatiserad åtkomst, så användningen sker ' +
    'på din egen risk: CAPTCHA, tillfällig spärr eller kontoåtgärd kan förekomma. ' +
    'Tillägget agerar aldrig i ditt namn – det läser, sparar lokalt och öppnar sidor åt dig.';

  AOM.STATUS = STATUS;
  AOM.STATUS_LABEL = STATUS_LABEL;
  AOM.MSG = MSG;
  AOM.STORAGE_KEYS = STORAGE_KEYS;
  AOM.ALARM_SYNC = ALARM_SYNC;
  AOM.MARKETPLACES = MARKETPLACES;
  AOM.DEFAULT_SETTINGS = DEFAULT_SETTINGS;
  AOM.DEFAULT_SYNC_STATE = DEFAULT_SYNC_STATE;
  AOM.DISCLAIMER = DISCLAIMER;
  AOM.RISK_NOTICE = RISK_NOTICE;
  AOM.ORDER_HISTORY_URL = 'https://www.amazon.se/gp/css/order-history';
  AOM.RETURNS_URL = 'https://www.amazon.se/spr/returns/list';
})(typeof self !== 'undefined' ? self : globalThis);
