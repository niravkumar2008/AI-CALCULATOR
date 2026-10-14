# Geography app data (`core/geo_data.cpp`)

`make_geo_data.py` turns Natural Earth's public-domain map data into the const tables the
Geography app (`core/geo.h`, `core/geo.cpp`) draws from. The generated file is committed;
the raw downloads are not.

## Source and licence

- **Natural Earth** (naturalearthdata.com), via the `nvkelso/natural-earth-vector` GitHub repo,
  `master` branch, downloaded 2026-10-11:
  - `geojson/ne_110m_admin_0_countries.geojson`: outlines, `NAME`, ISO codes, `CONTINENT`, label points
  - `geojson/ne_110m_populated_places.geojson`: capitals (`FEATURECLA` "Admin-0 capital")
  - `geojson/ne_10m_populated_places_simple.geojson`: the six capitals the 110m file lacks
- Licence: **public domain** ("No permission is needed to use Natural Earth"). No attribution is
  required; we credit it anyway.

## Regenerate

```
mkdir %TEMP%\ne && cd %TEMP%\ne
curl -LO https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_110m_admin_0_countries.geojson
curl -LO https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_110m_populated_places.geojson
curl -LO https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_10m_populated_places_simple.geojson
cd <repo>
python tools/geo/make_geo_data.py %TEMP%\ne core/geo_data.cpp
```

Python 3, standard library only. The output is deterministic. The script prints the table sizes.
After changing the data, run the geo tests (`tests/geo_tests.cpp`, `runGeoTests()`), because the
capital spot checks and the size budget are pinned there.

## What the generator does

- Keeps all **177** features, including Antarctica, Fr. S. Antarctic Lands, and territories
  such as Greenland, Puerto Rico, New Caledonia and the Falkland Is. Names are Natural Earth's
  short English `NAME` (for example "Dem. Rep. Congo" and "Bosnia and Herz."). Disputed areas
  (W. Sahara, Palestine, Kosovo, N. Cyprus, Somaliland, Taiwan) keep Natural Earth's default
  naming and borders.
- **Code**: `ISO_A3`, or `ISO_A3_EH` where Natural Earth puts -99 (France, Norway). Natural Earth's
  `ADM0_A3` is used where neither exists: N. Cyprus `CYN`, Somaliland `SOL` and Kosovo `KOS`
  (`isoOfficial = false`, and the card says "not ISO").
- **Outlines**: Douglas-Peucker at 0.12 degrees, islands under 0.05 deg² dropped (a country always
  keeps its largest ring), quantized to int16 degrees ×100. Holes stay as rings (Lesotho inside
  South Africa) and are handled by the even-odd rule. Result: 288 rings, 7,589 points.
- **Zoom box** per country: the largest ring plus any ring within 3 degrees of it. This keeps France's
  zoom on the mainland and Corsica, not French Guiana, and the USA's on the lower 48.
- Rows are sorted by name with accents folded, so the alphabetical list is the table order.

## Capital choices

Capitals use the 110m "Admin-0 capital" with `ADM0CAP = 1` unless the list below overrides it.
Where Natural Earth lists more than one capital, or its flag is not the de jure capital, we keep:

| Country | Kept | Others / note |
|---|---|---|
| South Africa | **Pretoria** (executive) | Cape Town (legislative), Bloemfontein (judicial) |
| Bolivia | **Sucre** (constitutional) | La Paz is the seat of government |
| Tanzania | **Dodoma** (de jure) | NE 110m flags Dar es Salaam |
| Benin | **Porto-Novo** (constitutional) | Cotonou is the seat of government |
| Côte d'Ivoire | **Yamoussoukro** (official) | Abidjan (economic) |
| Sri Lanka | **Sri Jayawardenepura Kotte** | Colombo (commercial) |
| Netherlands | **Amsterdam** (constitutional) | The Hague is the seat of government |
| Malaysia | **Kuala Lumpur** (official) | Putrajaya (administrative) |
| eSwatini | **Mbabane** (administrative) | Lobamba (royal/legislative) |
| Israel | **Jerusalem** | Natural Earth's default, kept as is |
| Chile, Nigeria, Myanmar, Philippines, Japan, Morocco | Santiago, Abuja, Naypyidaw, Manila, Tokyo, Rabat | NE also lists an "alt" capital |
| W. Sahara | **Bir Lehlou** | the only capital NE gives the feature ("Admin-0 capital alt") |

These come from the 10m simple file because the 110m file lacks them: Falkland Is. → Stanley,
Greenland → Nuuk, Puerto Rico → San Juan, Palestine → Ramallah (NE's "Admin-0 region capital"),
New Caledonia → Nouméa, S. Sudan → Juba.

**Renamed** (Natural Earth's position is kept): Denmark → **Copenhagen**, Natural Earth's `NAME_EN`,
because its `NAME` is "København". Kazakhstan → **Astana**: the city was renamed back from
Nur-Sultan in September 2022, and Natural Earth still uses the old name.

**No capital**: Antarctica, Fr. S. Antarctic Lands and N. Cyprus. Natural Earth has no capital
point for them. They appear on the map and in the list, but never in the quiz.

Capital names are unique across the table, which the quiz relies on and the tests check. Whitespace
is normalized: NE writes "Washington,  D.C." with two spaces. Coordinates are Natural Earth's
city points (×100), and 28 of them are spot-checked in the tests to within 1 degree.

## Size

The tables take about **41.7 KB** on a 32-bit target: points 30.4 KB, rings 1.2 KB, country rows
177 × 36 B = 6.4 KB, and strings 3.8 KB. `geo::dataBytes()` computes this, and a test keeps it
under 60 KB.
