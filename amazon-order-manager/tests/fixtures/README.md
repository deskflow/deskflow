# Testfixturer

**Dessa filer är INTE riktig HTML från Amazon.** De är syntetiska
approximationer som efterliknar de strukturmönster parsern förlitar sig på
(data-attribut, etiketttexter, produktlänkar, leveransboxar). De finns för att
kunna regressionstesta parsningslogiken utan ett inloggat konto.

Innan parsern kan anses klar enligt arbetsorder avsnitt 3 punkt 6 måste den
kalibreras och verifieras mot **sparad HTML från det egna kontot** – minst
5–10 riktiga ordrar av olika typ. Att testerna här är gröna säger bara att
logiken fungerar mot de mönster vi antagit, inte att antagandena stämmer med
dagens amazon.se.
