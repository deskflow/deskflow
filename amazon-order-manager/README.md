# Orderkoll – orderhanterare för Amazon.se (Chrome-tillägg, MV3)

Oberoende tillägg som läser **ditt eget** Amazon.se-konto med din egen inloggade
session och håller ordning på: status per artikel, mottagningsbekräftelse,
avvikelser (återbetalning utan begärd retur), de tre returfönstren, prisjämförelse
mot andra Amazon-marknader, lagerbevakning och ett lokalt bevisarkiv.

> **Detta tillägg är oberoende och inte skapat, godkänt av eller anslutet till Amazon.**

Byggt efter arbetsordern i `../arbetsorder/` (Amazon.se Orderhanterare). Namnet
"Orderkoll" är ett förslag – öppet beslut 9.5 – och ändras på ett ställe:
`manifest.json`.

---

## 1. Risk och policyläge – läs detta först

Amazons Conditions of Use förbjuder uttryckligen automatiserad åtkomst
("robot, spider, scraper, or other automated means"). Tillägget gör inget
brottsligt – det läser ditt eget konto med din egen session – men det bryter
sannolikt mot villkoren. Etablerade verktyg med samma grundfunktion har funnits
öppet i flera år utan att enskilda användare vid måttlig, personlig användning
har drabbats, men risken är inte noll: CAPTCHA, tillfällig spärr eller i värsta
fall kontoåtgärd är möjliga.

Vad koden gör åt saken:

- **En enda kö** för allt bakgrundsarbete, sekventiell, med slumpad paus
  (3–9 s, konfigurerbart) mellan varje hämtning. Aldrig parallella anrop.
- **Ingen automatisering i ditt namn.** Ingen beställning läggs, inget meddelande
  skickas, ingen knapp klickas åt dig. Enbart läsning, lokal bearbetning och
  genvägar som öppnar rätt sida åt dig.
- **CAPTCHA/inloggningsvägg stoppar kön** och flaggas i popupen i stället för att
  tystas ner.
- Risktexten visas i Inställningar och måste kvitteras där.

---

## 2. Installation (olåst/unpacked)

1. Öppna `chrome://extensions` i Chrome 120 eller senare.
2. Slå på **Utvecklarläge**.
3. Klicka **Läs in okomprimerat** och välj mappen `amazon-order-manager/`.
4. Ikonen dyker upp i verktygsfältet. Öppna Inställningar och kvittera risktexten.
5. Gå till dina ordrar på amazon.se, öppna tillägget och tryck **Synka**.

Manifest V2 tas bort helt från Chrome Web Store 31 augusti 2026; det här är
MV3 från början.

---

## 3. Så är det byggt

```
manifest.json               MV3: storage, unlimitedStorage, alarms, notifications
background/service-worker.js  alarm, kö, hämtningar, notiser, badge, all lagring
content-scripts/
  order-history.js          parsning av /gp/css/order-history
  order-detail.js           antal, styckpris, säljar-ID, betalsätt, returfrist
  returns-page.js           registrerade returer/återbetalningar
  product-page.js           identitet, "Jämför pris", "Spara bevis", "Bevaka lager"
  overlay/                  injicerad instrumentpanel i Shadow DOM
lib/
  parser-utils.js           fallback-selektorer + ParseReport (loggar vad som bar)
  status-model.js           fyra statusar, aldrig en gissad femte
  dates.js                  de tre returfönstren, tydligt åtskilda
  storage.js                chrome.storage.local, användarfält vinner alltid
  db.js                     IndexedDB för bevisarkivet
  sync-queue.js             en kö, sekventiell, slumpad paus, CAPTCHA-stopp
  anomaly-detection.js      avvikelseregeln i 5.3
  price-match.js            ASIN -> EAN -> modellnr -> bildhash, med verifiering
  image-hash.js             dHash (OffscreenCanvas i service workern)
  proof-capture.js          bevispaket, SHA-256 per fil + samlingshash
  proof-diff.js             sparad ögonblicksbild mot dagens sida
  fx.js                     ECB:s dagliga referenskurser
  zip.js                    egen ZIP-skrivare (inga tredjepartsberoenden)
  chart.js, dashboard.js    utgiftsdiagram och den gemensamma panelen
popup/, options/, pages/    popup, inställningar, fristående panel, bevisvy
```

Inga tredjepartsberoenden. Ingen byggkedja – mappen laddas som den är.

### Datamodellens viktigaste regler

- `quantity` och `unitPrice` finns **bara** på artikelnivå och hämtas från
  orderdetaljsidan. Historikvyn sätter dem till `null` – okänt visas som okänt.
- `sellerId` är sanningen, `sellerNameSnapshot` är namnet vid tillfället.
- Användarens fält (`userReceivedConfirmed`, `userConfirmedReturnOrRefundRequested`,
  `userHasDefect`) skrivs aldrig över av en ny skrapning.
- Amazons returpolicy **läses av sidan**. Går den inte att läsa står det "okänt".

### De tre returfönstren hålls isär (5.4)

| Fönster | Källa | Regel |
|---|---|---|
| Amazons returpolicy | avläst från sidan | frivilligt åtagande, varierar – hårdkodas aldrig |
| Ångerrätt | lag (2005:59) | 14 dagar, räknat från dagen efter mottagandet |
| Reklamationsrätt | lag (2022:260) | 3 år, **bara vid fel** – visas bara för ordrar du markerat "har fel" |
| Bank-/kortreklamation | din bank | ingen lagfrist, inget standardvärde – anges per betalsätt i Inställningar |

---

## 4. Verifieringsläge – vad som faktiskt är bevisat

Automatiska tester kör i riktig Chromium (Playwright) och i Node:

```bash
cd amazon-order-manager
npm install          # enda beroendet är Playwright, och bara för testerna
npm test             # ren logik i Node (19 tester)
npm run test:dom     # content scripts mot fixturer i riktig Chromium (7 tester)
npm run test:ext     # tillägget laddat olåst i Chromium (11 tester)
```

Alla 37 testerna är gröna i den här miljön. Installationstesterna kräver
Playwrights fullständiga Chromium (`channel: 'chromium'`) – headless shell
laddar inte tillägg.

| Grind (arbetsorder 8) | Status | Bevis |
|---|---|---|
| 1. Manifest och grundskelett | **Godkänd** | tillägget laddas olåst, MV3-manifest validerat, service worker startar, alarm 30 min, ikon läses |
| 2. Parsning av orderhistorik | **Delvis** | logiken verifierad mot syntetiska fixturer (3 ordrar: enstaka, blandad, avbruten). **Kräver kalibrering mot ditt konto.** |
| 3. Orderdetalj + antal | **Delvis** | antal, styckpris, betalsätt, returfrist och kontrollsumma (2×149,25 + 350 = 648,50) verifierade mot fixtur. Kräver riktiga ordrar. |
| 4. Lagring | **Godkänd** | skrivning och återläsning i den riktiga tilläggskontexten |
| 5. Popup med grunddata | **Godkänd** | popupen visar sparad data på en icke-Amazon-sida, synk-knappen gråad med exakt hjälptext |
| 6. Statusmodell + mottagningsbekräftelse | **Godkänd** | posten lämnar "Att bekräfta"-kön, ordern finns kvar med bekräftelsemarkering |
| 7. Returer + avvikelselogik | **Godkänd (logik)** | båda vägarna testade: kryss = ingen flagga, utan kryss + återbetalning = flagga. Returer-sidans selektorer kräver kalibrering. |
| 8. Bakgrundssynk + kö | **Godkänd (kö)** | kön kör sekventiellt (max 1 samtidig), ≥300 ms paus mellan jobb, pausen syns i loggen, CAPTCHA stoppar kön. Själva bakgrundsfliken kräver inloggat konto. |
| 9. Overlay i Shadow DOM | **Godkänd** | sidans `* { color:red !important; font-size:33px }` läcker inte in, tilläggets CSS läcker inte ut |
| 10. Prisjämförelse | **Delvis** | kedjan testad med stubbade svar: ASIN-träff, "ej i katalogen", och EAN-träff med fel kapacitet som blir "variant osäker". Riktiga hämtningar mot .de/.fr m.fl. återstår. |
| 11. Bevissäkring | **Delvis** | paket skapas, hashas (SHA-256 per fil + samlingshash), lagras i IndexedDB, indexeras, och manipulation upptäcks. Köpflödet kräver ett riktigt köp. |
| 12. Notiser + färskhet + disclaimer | **Godkänd (notiser)** | tre förändringar i en omgång ger **en** notis ("3 uppdateringar"). Färskhetsetikett och disclaimer finns i alla vyer. |
| 13. Helhetsgenomgång | **Kvarstår** | kräver en dag på ett riktigt konto |

### Vad jag inte har kunnat verifiera – och varför

Jag har inte tillgång till ett inloggat Amazon.se-konto. Därför gäller:

1. **Selektorerna är kalibrerade mot syntetiska fixturer, inte mot amazon.se.**
   Fixturerna i `tests/fixtures/` efterliknar de strukturmönster parsern bygger
   på (`data-order-id`, etiketterna "Orderlagd"/"Summa"/"Såld av",
   `data-component="shipments"`, produktlänkar `/dp/`), men de **är inte** riktig
   Amazon-HTML. Att testerna är gröna säger att logiken fungerar mot antagandena –
   inte att antagandena stämmer med dagens amazon.se.
2. **URL-mönstren för returer-sidan är en kvalificerad gissning.** Manifestet
   matchar flera varianter (`/spr/returns*`, `/gp/orc/returns*`, `/gp/css/returns*`).
   Kontrollera vilken din marknad faktiskt använder och stryk resten.
3. **Inget riktigt köp har testats** genom bevissäkringens primärflöde.

### Så kalibrerar du selektorerna (arbetsorder 3)

1. Logga in, gå till `amazon.se/gp/css/order-history`, öppna DevTools och kör
   `copy(document.documentElement.outerHTML)`. Klistra in i en fil.
2. Gör samma sak för en orderdetaljsida, returer-sidan och en produktsida.
3. Lägg filerna i `tests/fixtures/` (ersätt de syntetiska) och kör DOM-testerna.
4. Öppna konsolen på riktiga sidor: varje parsning loggar en `ParseReport` som
   visar exakt vilken strategi som bar varje fält och vilka som saknades. Fält som
   står som saknade är där selektorlistan behöver kompletteras.
5. Kör mot 5–10 riktiga ordrar av olika typ innan steg 2 och 3 anses klara.

---

## 5. Medvetna avsteg från arbetsordern

- **PDF-export sker via utskriftsvyn** (`pages/proof.html` → "Skriv ut / spara som
  PDF") i stället för en egen PDF-generator. En handskriven PDF-skrivare utan
  bibliotek hade varit en overifierbar riskkonstruktion; utskrift till PDF ger
  samma fil med ett klick extra. ZIP-exporten (råfiler + manifest med alla hashar)
  är däremot helt egen och verifierad.
- **"Dold bakgrundsflik" finns inte i Chrome.** Bakgrundssynken öppnar antingen en
  inaktiv flik i nuvarande fönster eller ett minimerat fönster (val i
  Inställningar). Fliken stängs när synken är klar.
- **Bevisexport och blobbar hanteras i bevisvyn**, inte i service workern, eftersom
  `URL.createObjectURL` inte finns i service workers och blobbar inte kan skickas
  genom `chrome.runtime.sendMessage`.
- **Nedskalning av gamla bevis** är förberedd som inställning men avstängd som
  standard (öppet beslut 9.4) – ingen bild skalas ned bakom ryggen på dig.
- **Ingen i18n-katalog.** Gränssnittet är hårdkodad svenska (arbetsordern lämnade
  valet öppet). `default_locale` är därför borttaget ur manifestet – med den kvar
  vägrar Chrome att läsa in tillägget utan `_locales/sv/messages.json`.

## 6. Öppna beslut och valda standardvärden (arbetsorder 9)

| Beslut | Standard i koden | Ändras i |
|---|---|---|
| Synkintervall | 30 minuter | Inställningar |
| Bankreklamationsfrist | **inget standardvärde** | Inställningar, per betalsätt |
| Marknader i prisjämförelsen | .de, .fr, .it, .es, .nl, .co.uk | Inställningar (påverkar `host_permissions`) |
| Nedskalning av bevis | avstängd | Inställningar |
| Namn och ikon | "Orderkoll" + egen kvitto/bock-ikon | `manifest.json`, `tools/make-icons.py` |

## 7. Integritet

All data ligger lokalt i din webbläsare: metadata i `chrome.storage.local`,
bevisarkivet i IndexedDB med `unlimitedStorage`. Ingenting skickas till Amazon,
till oss eller till någon tredje part. Enda utgående anrop utöver amazon.se är
ECB:s publika växelkursfil, och den innehåller ingen information om dig.
