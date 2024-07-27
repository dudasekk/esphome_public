#include <string.h>
#include "esphome.h"

// Only C mode
#define POLLTIMEMS ( 80 * 1000 )
#define READTIMEOUT ( 5 * 1000 )
#define CMDBAUD 300
#define BITS 7
#define STARTB 1
#define STOPB 1
#define PARITY UART_CONFIG_PARITY_EVEN 
#define IDCMD "/?!\r\n"
// ACknowledge without change speed
#define ACKMSG "x06""000\r\n"
#define STX '\x02'
#define ETX '\x03'
#define ACK '\x06'
#define UNITSEP '*'

uint16_t const BAUD_RATES[] = {
  /* 0 */ 300,
  /* 1, A */ 600,
  /* 2, B */ 1200,
  /* 3, C */ 2400,
  /* 4, D */ 4800,
  /* 5, E */ 9600,
  /* 6, F */ 19200
};

/* *********************************************
The code consists of (up to) 6 group sub-identifiers marked by letters A to F. All these may or may not be present in the identifier (e.g. groups A and B are often omitted). In order to decide to which group the sub-identifier belongs, the groups are separated by unique separators:
A-B:C.D.E*F
- The A group specifies the medium (0=abstract objects, 1=electricity, 6=heat, 7=gas, 8=water ...)
- The B group specifies the channel. Each device with multiple channels generating measurement results, can separate the results into the channels.
- The C group specifies the physical value (current, voltage, energy, level, temperature, ...)
- The D group specifies the quantity computation result of specific algorythm
- The E group specifies the measurement type defined by groups A to D into individual measurements (e.g. switching ranges)
- The F group separates the results partly defined by groups A to E. The typical usage is the specification of individual time ranges.
C.1.0 	Meter serial number
0.0.0 	Customer number = barcode
0.3.0 	Active energy meter constant
F.F.0 	Error meter status
1.8.0 	Positive active energy (A+) total [kWh]
1.8.1 	Positive active energy (A+) in tariff T1 [kWh]
1.8.2 	Positive active energy (A+) in tariff T2 [kWh]
21.8.0 	Positive active energy (A+) in phase L1 total [kWh]
41.8.0 	Positive active energy (A+) in phase L2 total [kWh]
61.8.0 	Positive active energy (A+) in phase L3 total [kWh]
2.8.0 	Negative active energy (A+) total [kWh]
22.8.0 	Negative active energy (A-) in phase L1 total [kWh]
42.8.0 	Negative active energy (A-) in phase L2 total [kWh]
62.8.0 	Negative active energy (A-) in phase L3 total [kWh]
C.8.1   Operating period of tariff register t1. Format RRMMDDhhmm
C.8.2   Operating period of tariff register t2. Format RRMMDDhhmm
C.8.0   Operating period in total +a. Format RRMMDDhhmm
C.82.0  Operating period in total -a. Format RRMMDDhhmm
C.7.1   Number of power failures in phase L1
C.7.2   Number of power failures in phase L2
C.7.3   Number of power failures in phase L3
0.2.1 	SW version
C.2.1 	Event parameters change - timestamp
C.2.9   Date and time of the last read-out. Format RRMMDDhhmm
C.3.9   Number of trials of attacking by magnetic field (scaler, counter)
===============================================================================
"OBIS code
C.D.E"	Description EN	Highlight  allways	Description CZ
0.0.0	Device address 	1	Adresa zařízení
0.0.1	Customer Specific 0.0.1		Zákaznický údaj 0.0.1
0.0.2	Customer Specific 0.0.2		Zákaznický údaj 0.0.2
0.0.3	Customer Specific 0.0.3		Zákaznický údaj 0.0.3
0.0.4	Customer Specific 0.0.4		Zákaznický údaj 0.0.4
0.0.5	Customer Specific 0.0.5		Zákaznický údaj 0.0.5
0.1.0	MD reset counter 		Počet nulování registrů
0.1.1	Customer Specific 0.1.1		Zákaznický údaj 0.1.1
0.1.2	MD reset timestamp	1	Datum/čas nulování registrů
0.2.0	Firmware version		Verze firmware
0.2.0.1	Customer Specific 0.2.0.1		Zákaznický údaj 0.2.0.1
0.2.1	Parameters scheme ID		ID nebo název konfigurace (schematu parametrů)
0.2.2	Tariff program ID or name		ID nebo název terifního programu
0.2.7	Passive calendar name		Název kalendáře
0.3.0	Active energy meter constant (metrological LED)		Konstanta činné energie (LED)
0.3.1	Reactive energy meter constant (metrological LED)		Konstanta jalové energie (LED)
0.3.3	Active energy - output pulse		Výstupní impuls pro činnou energii
0.3.4	Reactive energy - output pulse		Výstupní impuls pro jalovou energii
0.4.1	Reading factor for energy		
0.4.2	Current transformer ratio (numerator)	1	Převod proudových transformátorů (primár)
0.4.3	Voltage transformer ratio (numerator)		Převod napěťových transformátorů (primár)
0.4.4	Current transformer ratio (denominator)		Převod proudových transformátorů (sekundár)
0.4.5	Voltage transformer ratio (denominator)		Převod napěťových transformátorů (sekundár)
0.8.0	Demand (medium power) period [mins]	1	Perioda odečtu středního výkonu
0.8.4	Load profile period [min] (option)		
0.9.0	Time expired since last end of billing period		Doba od konce posledního fakturačního období
0.9.1	Current time	1	Aktuální čas
0.9.2	Current date 	1	Aktuální datum
0.9.4	Date and Time		Datum a čas
0.9.5	Day of week		Den v týdnu
1.2.0	Positive active cumulative maximum demand (A+) total [kW]		Maximum činné spotřeby kumulativní (A+) total
1.2.1	Positive active cumulative maximum demand (A+) in tariff T1 [kW]		Maximum činné spotřeby kumulativní (A+) v tarifu T1
1.2.2	Positive active cumulative maximum demand (A+) in tariff T2 [kW]		Maximum činné spotřeby kumulativní (A+) v tarifu T2
1.2.3	Positive active cumulative maximum demand (A+) in tariff T3 [kW]		Maximum činné spotřeby kumulativní (A+) v tarifu T3
1.2.4	Positive active cumulative maximum demand (A+) in tariff T4 [kW]		Maximum činné spotřeby kumulativní (A+) v tarifu T4
1.36.0	Demand threshold counter 		
1.36.0	Demand threshold counter 		
1.36.0	Demand threshold counter 		
1.36.0	Demand threshold counter 		
1.4.0	Positive active demand in a current demand period (A+) [kW]		Činná spotřeba (A+) v aktuálním období
1.5.0	Positive active demand in the last completed demand period (A+) [kW]		Činná spotřeba (A+) v poslední ukončeném období
1.5.1	Import active tariff 1 power at apparent demand		***
1.5.2	Import active tariff 2 power at apparent demand		***
1.5.3	Import active tariff 3 power at apparent demand		***
1.5.4	Import active tariff 4 power at apparent demand		***
1.6.0	Positive active maximum demand (P+) total [kW]	1	Maximum činné spotřeby (P+) celkově
1.6.1	Positive active maximum demand (P+) in tariff T1 [kW]		Maximum činné spotřeby (P+) v tarifu T1
1.6.2	Positive active maximum demand (P+) in tariff T2 [kW]		Maximum činné spotřeby (P+) v tarifu T2
1.6.3	Positive active maximum demand (P+) in tariff T3 [kW]		Maximum činné spotřeby (P+) v tarifu T3
1.6.4	Positive active maximum demand (P+) in tariff T4 [kW]		Maximum činné spotřeby (P+) v tarifu T4
1.7.0	Positive active instantaneous power (A+) 	1	Aktuální činná spotřeba (P+)
1.8.0	Positive active energy (A+) total [kWh]	1	Spotřeba činné energie (A+) celkově
1.8.1	Positive active energy (A+) in tariff T1 [kWh]	1	Spotřeba činné energie (A+) v tarifu T1
1.8.2	Positive active energy (A+) in tariff T2 [kWh]		Spotřeba činné energie (A+) v tarifu T2
1.8.3	Positive active energy (A+) in tariff T3 [kWh]		Spotřeba činné energie (A+) v tarifu T3
1.8.4	Positive active energy (A+) in tariff T4 [kWh]		Spotřeba činné energie (A+) v tarifu T4
10.6.0	Apparent Maximum demand (export)		***
10.7.0	Apparent instantaneous power (S-) [kVA]		***
10.8.0	Apparent energy (S-) total [kVAh]		***
11.6.0	Maximum current (I max) [A]		***
11.7.0	Instantaneous current (I) [A]		***
12.7.0	Instantaneous voltage (U) [V]		***
13	Power factor (Applied Meters specific)	1	Účinník (zkrácený kód AMT)
13.25	Power factor average (AMT specific)		Průměrný účinník
13.7.0	Instantaneous power factor (overall)	1	Okamžitá hodnota účinníku
136.1.1	Manufacturer Specific 136.1.1		Specifický údaj výrobce 136.1.1
136.1.2	Manufacturer Specific 136.1.2		Specifický údaj výrobce 136.1.2
136.1.3	Manufacturer Specific 136.1.3		Specifický údaj výrobce 136.1.3
136.1.4	Manufacturer Specific 136.1.4		Specifický údaj výrobce 136.1.4
136.1.5	Manufacturer Specific 136.1.5		Specifický údaj výrobce 136.1.5
136.1.6	Manufacturer Specific 136.1.6		Specifický údaj výrobce 136.1.6
14	Frequency [Hz] (Applied Meters specific)		Frekvence (zkrácený kód AMT)
14.7.0	Frequency [Hz]		Okamžitá hodnota frekvence
15.2.0	Absolute active cumulative maximum demand (|A|) total [kW]		Absolutní činná spotřeba kumulativní (|A|) total
15.2.1	Absolute active cumulative maximum demand (|A|) in tariff T1 [kW]		***
15.2.2	Absolute active cumulative maximum demand (|A|) in tariff T2 [kW]		***
15.2.3	Absolute active cumulative maximum demand (|A|) in tariff T3 [kW]		***
15.2.4	Absolute active cumulative maximum demand (|A|) in tariff T4 [kW]		***
15.4.0	Absolute active demand in a current demand period (|A|) [kW]		***
15.5.0	Absolute active demand in the last completed demand period (|A|) [kW]		***
15.6.0	Absolute active maximum demand (|A|) total [kW]		***
15.6.1	Absolute active maximum demand (|A|) in tariff T1 [kW]		***
15.6.2	Absolute active maximum demand (|A|) in tariff T2 [kW]		***
15.6.3	Absolute active maximum demand (|A|) in tariff T3 [kW]		***
15.6.4	Absolute active maximum demand (|A|) in tariff T4 [kW]		***
15.7.0	Absolute active instantaneous power (|A|) [kW]		***
15.8.0	Absolute active energy (A+) total [kWh]		***
15.8.1	Absolute active energy (A+) in tariff T1 [kWh]		***
15.8.2	Absolute active energy (A+) in tariff T2 [kWh]		***
15.8.3	Absolute active energy (A+) in tariff T3 [kWh]		***
15.8.4	Absolute active energy (A+) in tariff T4 [kWh]		***
16.7.0	Sum active instantaneous power (A+ - A-) [kW]		***
16.8.	Sum active energy without reverse blockade (A+ - A-) in tariff T1 [kWh]		
16.8.0	Sum active energy without reverse blockade (A+ - A-) total [kWh]		***
16.8.2	Sum active energy without reverse blockade (A+ - A-) in tariff T2 [kWh]		***
16.8.3	Sum active energy without reverse blockade (A+ - A-) in tariff T3 [kWh]		***
16.8.4	Sum active energy without reverse blockade (A+ - A-) in tariff T4 [kWh]		***
2.2.0	Negative active cumulative maximum demand (P-) total [kW]		Maximum činné dodávky kumulativní (P-) celkově
2.2.1	Negative active cumulative maximum demand (P-) in tariff T1 [kW]		Maximum činné dodávky kumulativní (P-) v tarifu T1
2.2.2	Negative active cumulative maximum demand (P-) in tariff T2 [kW]		Maximum činné dodávky kumulativní (P-) v tarifu T2
2.2.3	Negative active cumulative maximum demand (P-) in tariff T3 [kW]		Maximum činné dodávky kumulativní (P-) v tarifu T3
2.2.4	Negative active cumulative maximum demand (P-) in tariff T4 [kW]		Maximum činné dodávky kumulativní (P-) v tarifu T4
2.4.0	Negative active demand in a current demand period (P-) [kW]		Činná dodávka (P-) v aktuálním období
2.5.0	Negative active demand in the last completed demand period (P-) [kW]		Činná dodávka (P-) v posledním ukončeném období
2.5.1	Export active tariff 1 power at apparent demand		***
2.5.2	Export active tariff 2 power at apparent demand		***
2.5.3	Export active tariff 3 power at apparent demand		***
2.5.4	Export active tariff 4 power at apparent demand		***
2.6.0	Negative active maximum demand (P-) total [kW]		Maximum činné dodávky (P-) celkově
2.6.1	Negative active maximum demand (P-) in tariff T1 [kW]		Maximum činné dodávky (P-) v tarifu T1
2.6.2	Negative active maximum demand (A-) in tariff T2 [kW]		Maximum činné dodávky (P-) v tarifu T2
2.6.3	Negative active maximum demand (P-) in tariff T3 [kW]		Maximum činné dodávky (P-) v tarifu T3
2.6.4	Negative active maximum demand (P-) in tariff T4 [kW]		Maximum činné dodávky (P-) v tarifu T4
2.7.0	Negative active instantaneous power (A-) [kW]		Okamžitá hodnota činné dodávky (P-)
2.8.0	Negative active energy (A-) total [kWh]		Dodávka činné energie (A-) celkem
2.8.1	Negative active energy (A+) in tariff T1 [kWh]		Dodávka činné energie (A-) v tarifu T1
2.8.2	Negative active energy (A+) in tariff T2 [kWh]		Dodávka činné energie (A-) v tarifu T2
2.8.3	Negative active energy (A+) in tariff T3 [kWh]		Dodávka činné energie (A-) v tarifu T3
2.8.4	Negative active energy (A+) in tariff T4 [kWh]		Dodávka činné energie (A-) v tarifu T4
21.7.0	Positive active instantaneous power (A+) in phase L1 [kW]		***
21.8.0	Positive active energy (A+) in phase L1 total [kWh]		***
22.7.0	Negative active instantaneous power (A-) in phase L1 [kW]		***
22.8.0	Negative active energy (A-) in phase L1 total [kWh]		***
23.7.0	Positive reactive instantaneous power (Q+) in phase L1 [kvar]		***
23.8.0	Phase L1 positive reactive energy (Q+) [kVArh]		***
24.7.0	Negative reactive instantaneous power (Q-) in phase L1 [kvar]		***
24.8.0	Phase L1 negative reactive energy (Q-) [kVArh]		***
29.7.0	Apparent instantaneous power (S+) in phase L1 [kVA]		***
3.2.0	Positive reactive cumulative maximum demand (Q+) total [kvar]		Maximum jalové spotřeby kumulativní (Q+) total
3.36.0	Demand threshold counter 		
3.36.0	Demand threshold counter 		
3.36.0	Demand threshold counter 		
3.4.0	Positive reactive demand in a current demand period (Q+) [kvar]		***
3.5.0	Positive reactive demand in the last completed demand period (Q+) [kvar]		***
3.6.0	Positive reactive maximum demand (Q+) total [kvar]	1	***
3.7.0	Positive reactive instantaneous power (Q+) [kvar]		***
3.8.0	Positive reactive energy (Q+) total [kvarh]	1	***
3.8.1	Positive reactive energy (Q+) in tariff T1 [kvarh]	1	***
3.8.2	Positive reactive energy (Q+) in tariff T2 [kvarh]		***
3.8.3	Positive reactive energy (Q+) in tariff T3 [kvarh]		***
3.8.4	Positive reactive energy (Q+) in tariff T4 [kvarh]		***
30.7.0	Instantaneous apparent export power of phase A		***
31	Current I1 (Applied Meters specific)		Proud I1 (zkrácený kód AMT)
31.25	Current I1 [A] average (AMT specific)		Proud I1 průměr [A]
31.6.0	Maximum current (I max) in phase L1 [A]	1	
31.7.0	Instantaneous current (I) in phase L1 [A]		Proud I1
32	Voltage U1 (Applied Meters specific)		Napětí U1 (zkrácený kód AMT)
32.25	Voltage U1 average (AMT specific)		Napětí U1 průměr [V]
32.32.0	Number of voltage drops (undervoltage counter) in phase L1		
32.7.0	Instantaneous voltage (U) in phase L1 [V]	1	Napětí U1
33.7.0	Instantaneous power factor in phase L1		
35.7.0	Absolute active instantaneous power (|A|) in phase L1 [kW]		
35.8.0	Absolute active energy (|A|) in phase L1 total [kWh]		
36.7.0	Sum active instantaneous power (A+ - A-) in phase L1 [kW]		
4.2.0	Negative reactive cumulative maximum demand (Q-) total [kvar]		Maximum jalové dodávky kumulativní (Q-) total
4.4.0	Negative reactive demand in a current demand period (Q-) [kvar]		***
4.5.0	Negative reactive demand in the last completed demand period (Q-) [kvar]		***
4.6.0	Negative reactive maximum demand (Q-) total [kvar]		***
4.7.0	Negative reactive instantaneous power (Q-) [kvar]	1	***
4.8.0	Negative reactive energy (Q-) total [kvarh]	1	***
4.8.1	Negative reactive energy (Q-) in tariff T1 [kvarh]	1	***
4.8.2	Negative reactive energy (Q-) in tariff T2 [kvarh]		***
4.8.3	Negative reactive energy (Q-) in tariff T3 [kvarh]		***
4.8.4	Negative reactive energy (Q-) in tariff T4 [kvarh]		***
41.7.0	Positive active instantaneous power (A+) in phase L2 [kW]		
41.8.0	Positive active energy (A+) in phase L2 total [kWh]		
42.7.0	Negative active instantaneous power (A-) in phase L2 [kW]		
42.8.0	Negative active energy (A-) in phase L2 total [kWh]		
43.7.0	Positive reactive instantaneous power (Q+) in phase L2 [kvar]		
43.8.0	Phase L2 positive reactive energy (Q+) [kVArh]		
44.7.0	Negative reactive instantaneous power (Q-) in phase L2 [kvar]		
44.8.0	Phase L2 negative reactive energy (Q-) [kVArh]		
49.7.0	Apparent instantaneous power (S+) in phase L2 [kVA]		
5.2.0	Reactive energy Q1 cumulative maximum demand, total [kVAr]		Jalová energie Q1 maximum spotřeby
5.2.1	Reactive energy Q1 cumulative maximum demand, tariff 1 [kVAr]		***
5.2.2	Reactive energy Q1 cumulative maximum demand, tariff 2 [kVAr]		***
5.2.3	Reactive energy Q1 cumulative maximum demand, tariff 3 [kVAr]		***
5.2.4	Reactive energy Q1 cumulative maximum demand, tariff 4 [kVAr]		***
5.4.0	Reactive demand in a current demand period in Q1 (Q1) [kvar]		***
5.5.0	Reactive demand in the last completed demand period in Q1 (Q1) [kvar]		***
5.6.0	Reactive energy Q1 maximum demand, total [kVAr]		***
5.6.1	Reactive energy Q1 maximum demand, tariff 1 [kVAr]		***
5.6.2	Reactive energy Q1 maximum demand, tariff 2 [kVAr]		***
5.6.3	Reactive energy Q1 maximum demand, tariff 3 [kVAr]		***
5.6.4	Reactive energy Q1 maximum demand, tariff 4 [kVAr]		***
5.7.0	Reactive power Q1		***
5.8.0	Imported inductive reactive energy in 1-st quadrant (Q1) total [kvarh]		***
5.8.1	Imported inductive reactive energy in 1-st quadrant (Q1) in tariff T1 [kvarh]		***
5.8.2	Imported inductive reactive energy in 1-st quadrant (Q1) in tariff T2 [kvarh]		***
5.8.3	Imported inductive reactive energy in 1-st quadrant (Q1) in tariff T3 [kvarh]		***
5.8.4	Imported inductive reactive energy in 1-st quadrant (Q1) in tariff T4 [kvarh]		***
50.7.0	Apparent instantaneous power (S-) in phase L2 [kVA]		
51	Current I2 (Applied Meters specific)		Proud I2 (zkrácený kód AMT)
51.25	Current I2 [A] average (AMT specific)		Proud I2 průměr [A]
51.6.0	Maximum current (I max) in phase L2 [A]	1	
51.7.0	Instantaneous current (I) in phase L2 [A]	1	Proud I2
52	Voltage U2 (Applied Meters specific)		Napětí U2 (zkrácený kód AMT)
52.25	Voltage U2 average (AMT specific)		Napětí U2 průměr [V]
52.32.0	Number of voltage drops (undervoltage counter) in phase L2		
52.7.0	Instantaneous voltage (U) in phase L2 [V]	1	Napětí U2
53.7.0	Instantaneous power factor in phase L2		
55.7.0	Absolute active instantaneous power (|A|) in phase L2 [kW]		
55.8.0	Absolute active energy (|A|) in phase L2 total [kWh]		
56.7.0	Sum active instantaneous power (A+ - A-) in phase L2 [kW]		
6.2.0	Reactive energy Q2 cumulative maximum demand, total [kVAr]		Jalová energie Q2 maximum spotřeby
6.2.1	Reactive energy Q2 cumulative maximum demand, tariff 1 [kVAr]		***
6.2.2	Reactive energy Q2 cumulative maximum demand, tariff 2 [kVAr]		***
6.2.3	Reactive energy Q2 cumulative maximum demand, tariff 3 [kVAr]		***
6.2.4	Reactive energy Q2 cumulative maximum demand, tariff 4 [kVAr]		***
6.4.0	Reactive demand in a current demand period in Q2 (Q2) [kvar]		***
6.5.0	Reactive demand in the last completed demand period in Q2 (Q2) [kvar]		***
6.6.0	Reactive energy Q2 maximum demand, total [kVAr]		***
6.6.1	Reactive energy Q2 maximum demand, tariff 1 [kVAr]		***
6.6.2	Reactive energy Q2 maximum demand, tariff 2 [kVAr]		***
6.6.3	Reactive energy Q2 maximum demand, tariff 3 [kVAr]		***
6.6.4	Reactive energy Q2 maximum demand, tariff 4 [kVAr]		***
6.7.0	Reactive power Q2		***
6.8.0	Imported capacitive reactive energy in 2-nd quadrant (Q2) total [kvarh]		***
6.8.1	Imported capacitive reactive energy in 2-nd quadr. (Q2) in tariff T1 [kvarh]		***
6.8.2	Imported capacitive reactive energy in 2-nd quadr. (Q2) in tariff T2 [kvarh]		***
6.8.3	Imported capacitive reactive energy in 2-nd quadr. (Q2) in tariff T3 [kvarh]		***
6.8.4	Imported capacitive reactive energy in 2-nd quadr. (Q2) in tariff T4 [kvarh]		***
61.7.0	Positive active instantaneous power (A+) in phase L3 [kW]		
61.8.0	Positive active energy (A+) in phase L3 total [kWh]		
62.7.0	Negative active instantaneous power (A-) in phase L3 [kW]		
62.8.0	Negative active energy (A-) in phase L3 total [kWh]		
63.7.0	Positive reactive instantaneous power (Q+) in phase L3 [kvar]		
63.8.0	Phase L3 positive reactive energy (Q+) [kVArh]		
64.7.0	Negative reactive instantaneous power (Q-) in phase L3 [kvar]		
64.8.0	Phase L3 negative reactive energy (Q-) [kVArh]		
69.7.0	Apparent instantaneous power (S+) in phase L3 [kVA]		
7.2.0	Reactive energy Q3 cumulative maximum demand, total [kVAr]		Jalová energie Q3 maximum spotřeby
7.2.1	Reactive energy Q3 cumulative maximum demand, tariff 1 [kVAr]		***
7.2.2	Reactive energy Q3 cumulative maximum demand, tariff 2 [kVAr]		***
7.2.3	Reactive energy Q3 cumulative maximum demand, tariff 3 [kVAr]		***
7.2.4	Reactive energy Q3 cumulative maximum demand, tariff 4 [kVAr]		***
7.4.0	Reactive demand in a current demand period in Q3 (Q3) [kvar]		***
7.5.0	Reactive demand in the last completed demand period in Q3 (Q3) [kvar]		***
7.6.0	Reactive energy Q3 maximum demand, total [kVAr]		***
7.6.1	Reactive energy Q3 maximum demand, tariff 1 [kVAr]		***
7.6.2	Reactive energy Q3 maximum demand, tariff 2 [kVAr]		***
7.6.3	Reactive energy Q3 maximum demand, tariff 3 [kVAr]		***
7.6.4	Reactive energy Q3 maximum demand, tariff 4 [kVAr]		***
7.7.0	Reactive power Q3		***
7.8.0	Exported inductive reactive energy in 3-rd quadrant (Q3) total [kvarh]		***
7.8.1	Exported inductive reactive energy in 3-rd quadrant (Q3) in tariff T1 [kvarh]		***
7.8.2	Exported inductive reactive energy in 3-rd quadrant (Q3) in tariff T2 [kvarh]		***
7.8.3	Exported inductive reactive energy in 3-rd quadrant (Q3) in tariff T3 [kvarh]		***
7.8.4	Exported inductive reactive energy in 3-rd quadrant (Q3) in tariff T4 [kvarh]		***
70.7.0	Apparent instantaneous power (S-) in phase L3 [kVA]		
71	Current I3 (Applied Meters specific)		Proud I3 (zkrácený kód AMT)
71.25	Current I3 [A] average (AMT specific)		Proud I3 průměr [A]
71.6.0	Maximum current (I max) in phase L3 [A]	1	
71.7.0	Instantaneous current (I) in phase L3 [A]	1	Proud I3
72	Voltage U3 (Applied Meters specific)		Napětí U3 (zkrácený kód AMT)
72.25	Voltage U3 average (AMT specific)		Napětí U3 průměr [V]
72.32.0	Number of voltage drops (undervoltage counter) in phase L3		
72.7.0	Instantaneous voltage (U) in phase L3 [V]	1	Napětí U3
73.7.0	Instantaneous power factor in phase L3		
75.7.0	Absolute active instantaneous power (|A|) in phase L3 [kW]		
75.8.0	Absolute active energy (|A|) in phase L3 total [kWh]		
76.7.0	Sum active instantaneous power (A+ - A-) in phase L3 [kW]		
8.2.0	Reactive energy Q4 cumulative maximum demand, total [kVAr]		Jalová energie Q4 maximum spotřeby
8.2.1	Reactive energy Q4 cumulative maximum demand, tariff 1 [kVAr]		***
8.2.2	Reactive energy Q4 cumulative maximum demand, tariff 2 [kVAr]		***
8.2.3	Reactive energy Q4 cumulative maximum demand, tariff 3 [kVAr]		***
8.2.4	Reactive energy Q4 cumulative maximum demand, tariff 4 [kVAr]		***
8.4.0	Reactive demand in a current demand period in Q4 (Q4) [kvar]		***
8.5.0	Reactive demand in the last completed demand period in Q4 (Q4) [kvar]		***
8.6.0	Reactive energy Q4 maximum demand, total [kVAr]		***
8.6.1	Reactive energy Q4 maximum demand, tariff 1 [kVAr]		***
8.6.2	Reactive energy Q4 maximum demand, tariff 2 [kVAr]		***
8.6.3	Reactive energy Q4 maximum demand, tariff 3 [kVAr]		***
8.6.4	Reactive energy Q4 maximum demand, tariff 4 [kVAr]		***
8.7.0	Reactive power Q4		***
8.8.0	Exported capacitive reactive energy in 4-th quadrant (Q4) total [kvarh]		***
8.8.1	Exported capacitive reactive energy in 4-th quadr. (Q4) in tariff T1 [kvarh]		***
8.8.2	Exported capacitive reactive energy in 4-th quadr. (Q4) in tariff T2 [kvarh]		***
8.8.3	Exported capacitive reactive energy in 4-th quadr. (Q4) in tariff T3 [kvarh]		***
8.8.4	Exported capacitive reactive energy in 4-th quadr. (Q4) in tariff T4 [kvarh]		***
9.2.0	Apparent cumulative maximum demand (S+) total [kVA]		Maximum zdánlivé spotřeby (S+) total
9.4.0	Apparent demand in a current demand period (S+) [kVA]		***
9.5.0	Apparent demand in the last completed demand period (S+) [kVA]		***
9.6.0	Apparent maximum demand (S+) total [kVA]		***
9.6.0	Apparent Maximum demand (import)		***
9.7.0	Apparent instantaneous power (S+) [kVA]		***
9.8.0	Apparent energy (S+) total [kVAh]		***
9.8.1	Apparent energy (S+) in tariff T1 [kVAh]		***
9.8.2	Apparent energy (S+) in tariff T2 [kVAh]		***
9.8.3	Apparent energy (S+) in tariff T3 [kVAh]		***
9.8.4	Apparent energy (S+) in tariff T4 [kVAh]		***
91.6.0	Maximum current (I max) in neutral [A]		
91.7.0	Instantaneous current (I) in neutral [A]		
94.49.2.15	EON country specific 94.49.2.15		Zákaznický údaj  94.49.2.15
94.49.2.16	EON country specific 94.49.2.16		Zákaznický údaj  94.49.2.16
94.49.2.17	EON country specific 94.49.2.17		Zákaznický údaj  94.49.2.17
94.49.2.2	EON country specific 94.49.2.2		Zákaznický údaj  94.49.2.2
94.49.2.21	EON country specific 94.49.2.21		Zákaznický údaj  94.49.2.21
C.1.0	Meter serial number	1	Výrobní číslo přístroje
C.1.1	IEC Device address		Adresa přístroje
C.1.2	Parameters file code		Kód souboru paramtrů
C.1.3	Customer Specific C.1.3		Zákaznický údaj C.1.3
C.1.4	Parameters check sum		
C.1.5	Firmware built date		
C.1.6	Firmware check sum		
C.2.0	Event parameters change - counter	1	Počet změn nastavení parametrů
C.2.1	Event parameters change - timestamp	1	Datum/čas poslední změny parametrů
C.2.10	Customer Specific C.2.10		Zákaznický údaj C.2.10
C.2.11	Customer Specific C.2.11		Zákaznický údaj C.2.11
C.2.7	Customer Specific C.2.7		Zákaznický údaj C.2.7
C.2.9	Last read-out timestamp	1	Čas posledního odečtu (RS485)
C.3.7	Last terminal cover open - timestamp		Čas posledního otevření krytu svorkovnice
C.3.8	Last meter cover open - timestamp		Čas posledního otevření krytu elektroměru
C.3.9	Last attempt to hack with a magnet		Čas posledního napadení magnetem
C.50.1	Firmware CRC		CRC firmware
C.50.2	Last attempt to unauthorized access		Čas posledního pokusu o nepovolený přístup
C.51.1	Event terminal cover opened - counter		Počitadlo otevření krytu svorkovnice
C.51.13	Event power up - counter		Počitadlo obnovení napájení
C.51.14	Event power up - timestamp		Čas posledního obnovení napájení
C.51.15	Event RTC (Real Time Clock) set - counter		
C.51.16	Event RTC (Real Time Clock) set - timestamp		
C.51.2	Event terminal cover opened - timestamp		
C.51.21	Event terminal cover closed - counter		
C.51.22	Event terminal cover closed - timestamp		
C.51.23	Event main cover closed - counter		
C.51.24	Event main cover closed - timestamp		
C.51.25	Event log-book 1 erased - counter		
C.51.26	Event log-book 1 erased - timestamp		
C.51.27	Event fraud start - counter		
C.51.28	Event fraud start - timestamp		
C.51.29	Event fraud stop - counter		
C.51.3	Event main cover opened - counter		
C.51.30	Event fraud stop - timestamp		
C.51.4	Event main cover opened - timestamp		
C.51.5	Event magnetic field detection start - counter		
C.51.6	Event magnetic field detection start - timestamp		
C.51.7	Event reverse power flow - counter		
C.51.8	Event reverse power flow - timestamp		
C.53.1	Tamper 1 energy register		
C.53.10	Tamper 5 time counter register		
C.53.11	Tamper 5 energy register		
C.53.2	Tamper 2 energy register		
C.53.3	Tamper 3 energy register		
C.53.4	Tamper 4 energy register		
C.53.5	Tamper 1 time counter register		
C.53.6	Tamper 2 time counter register		
C.53.7	Tamper 3 time counter register		
C.53.9	Tamper 4 time counter register		
C.54.3	Customer Specific C.54.3		Zákaznický údaj C.54.3
C.54.4	Customer Specific C.54.4		Zákaznický údaj C.54.4
C.54.6	Customer Specific C.54.6		Zákaznický údaj C.54.6
C.56.1	Customer Specific C.56.1		Zákaznický údaj C.56.1
C.56.2	Customer Specific C.56.2		Zákaznický údaj C.56.2
C.56.3	Customer Specific C.56.3		Zákaznický údaj C.56.3
C.6.0	Power down time counter (internal battery run counter)	1	
C.6.1	Internal battery remaining capacity		Kapacita záložní baterie
C.60.9	Fraud flag		
C.7.0	Event power down - counter	1	Výpadek napájení přístroje (počitadlo)
C.7.1	L1 outage detection		Výpadek fáze L1
C.7.10	Event power down - timestamp		Výpadek napájení přístroje (datum/čas)
C.7.2	L2 outage detection		Výpadek fáze L2
C.7.3	L3 outage detection		Výpadek fáze L3
C.87.0	Active tariff		Aktivní tarif
C.99.2	Customer Specific C.99.2		Zákaznický údaj C.99.2
C.C.0	Terminal cover open	1	Otevření krytu svorkovnice
C.C.2	Number of hacking attacks by magnet		Počet pokusů o napadení magnetem
C.C.3	Meter cover open	1	Otevření krytu elektroměru
F.0.1	Status message		Status
F.F	Error status		Chybový kód obecný
F.F.0	Internal error message 1		Chybový kód 1
F.F.1	Internal error message 2		Chybový kód 2

*/


static const char *TAG = "iec62056elmeter";


class iec62056elmeter : public PollingComponent, public UARTDevice {
  public:
   // Electricity meter sensors ZPA5ZE312.v10_022
   TextSensor *Buffer = new TextSensor();
   TextSensor *DeviceType = new TextSensor();
   TextSensor *DeviceSN = new TextSensor();
   TextSensor *CustomerNr = new TextSensor();
   TextSensor *LastCfgTime = new TextSensor();
   TextSensor *SWVersion = new TextSensor();
   TextSensor *LastReadOutTime = new TextSensor();
   TextSensor *OperatingPeriodT1 = new TextSensor();
   TextSensor *OperatingPeriodT2 = new TextSensor();
   TextSensor *OperatingPeriodTotalPositiveEnergy = new TextSensor();
   TextSensor *OperatingPeriodTotalNegativeEnergy = new TextSensor();
   Sensor *ActiveEnergyConstant = new Sensor();
   Sensor *TotalEnergy = new Sensor();
   Sensor *PositiveEnergyT1 = new Sensor();
   Sensor *PositiveEnergyT2 = new Sensor();
   Sensor *TotalPositiveEnergyL1 = new Sensor();
   Sensor *TotalPositiveEnergyL2 = new Sensor();
   Sensor *TotalPositiveEnergyL3 = new Sensor();
   Sensor *TotalNegativeEnergy = new Sensor();
   Sensor *TotalNegativeEnergyL1 = new Sensor();
   Sensor *TotalNegativeEnergyL2 = new Sensor();
   Sensor *TotalNegativeEnergyL3 = new Sensor();
   Sensor *ErrorStatus = new Sensor();
   Sensor *PowerFailuresOnL1 = new Sensor();
   Sensor *PowerFailuresOnL2 = new Sensor();
   Sensor *PowerFailuresOnL3 = new Sensor();
   Sensor *NumOfMagneticAttacks = new Sensor();
   BinarySensor *DataRunning = new BinarySensor();

  public:
    iec62056elmeter(UARTComponent *parent ) : UARTDevice(parent) {}

  static iec62056elmeter* instance(UARTComponent *parent) {
      static iec62056elmeter* INSTANCE = new iec62056elmeter(parent);
      return INSTANCE;
     }

  uint8_t sendcmd(String outbytes) {
   DataRunning->publish_state(1);
   // Send command  to elmer
   write_str(outbytes.c_str()); 
   // clear input buffer
   flush();
   return(0);
  }

  int readline(int readch, char *buffer, int len)
  {
    static int pos = 0;
    int rpos;

    if (readch > 0) {
      switch (readch) {
        case '\n': // Ignore new-lines
          break;
        case '\r': // Return on CR
          rpos = pos;
          pos = 0;  // Reset position index ready for next time
          return rpos;
        case '\002': // STX - Return
          rpos = pos;
          pos = 0;  // Reset position index ready for next time
          return rpos;
        case '/': // clear chars before device definitions
          if(pos < 5) {
            pos = 0;
          }
        default:
          if (pos < len-1) {
            buffer[pos++] = readch;
            buffer[pos] = 0;
          }
      }
    }
    // No end of line has been found, so return -1.
    return -1;
  }


  void setup() override {
    this->set_update_interval(POLLTIMEMS);
    ESP_LOGCONFIG(TAG, "Setting up polling timer ...");
  }

  void setupdate(int polltm) {
    this->set_update_interval(polltm * 1000);
    ESP_LOGCONFIG(TAG, "Setting up polling timer to %d seconds", polltm);
  }

  void loop() override {
    const int max_line_length = 80;
    static char buffer[max_line_length];
    while (available()) {
      if(readline(read(), buffer, max_line_length) > 0) {
        Buffer->publish_state(buffer);
        ESP_LOGD(TAG, "Parse: %s ", buffer);
        parse(buffer);
      }

    }
  }

  void parse(char *buffer) {
    String mystring = String(buffer);
    mystring.trim();
    String code = mystring.substring(0,mystring.indexOf('('));
    String area = mystring.substring(mystring.indexOf('(')+1,mystring.indexOf(')'));
    String value = area.substring(0,area.indexOf('*'));
    String unit = area.substring(area.indexOf('*')+1);
    ESP_LOGD(TAG, "Code: %s", code.c_str());
    ESP_LOGD(TAG, "Area: %s", area.c_str());
    ESP_LOGD(TAG, "Value: %s", value.c_str());
    ESP_LOGD(TAG, "Unit: %s", unit.c_str());
    if(buffer[0] == '/' ) {
       ESP_LOGD(TAG, "DeviceType: %s ", mystring.substring(1).c_str());
       DeviceType->publish_state(mystring.substring(1).c_str());
       write_str(ACKMSG); 
       }
    if(code == "!") {
       ESP_LOGD(TAG, "End of telegram");
       DataRunning->publish_state(0);
       }
    if(code == "C.1.0" ) {
       ESP_LOGD(TAG, "SerialNr: %s ", value.c_str());
       DeviceSN->publish_state(value.c_str());
       }
    if(code == "C.2.1" ) {
       ESP_LOGD(TAG, "LastCfgTime: %s ", value.c_str());
       LastCfgTime->publish_state(value.c_str());
       }   
    if(code == "C.2.9" ) {
       ESP_LOGD(TAG, "LastReadOutTime: %s ", value.c_str());
       LastReadOutTime->publish_state(value.c_str());
       }   
    if(code == "C.3.9" ) {
       ESP_LOGD(TAG, "Nr Magnetic attack: %s ", value.c_str());
       NumOfMagneticAttacks->publish_state(value.toInt());
       }   
    if(code == "C.7.1" ) {
       ESP_LOGD(TAG, "Nr Pwr Failures on L1: %s ", value.c_str());
       PowerFailuresOnL1->publish_state(value.toInt());
       }   
    if(code == "C.7.2" ) {
       ESP_LOGD(TAG, "Nr Pwr Failures on L2: %s ", value.c_str());
       PowerFailuresOnL2->publish_state(value.toInt());
       }   
    if(code == "C.7.3" ) {
       ESP_LOGD(TAG, "Nr Pwr Failures on L3: %s ", value.c_str());
       PowerFailuresOnL3->publish_state(value.toInt());
       }   
    if(code == "C.8.0" ) {
       ESP_LOGD(TAG, "Operating Period Positive energy: %s ", value.c_str());
       OperatingPeriodTotalPositiveEnergy->publish_state(value.c_str());
       }   
    if(code == "C.8.1" ) {
       ESP_LOGD(TAG, "Operating Period T1: %s ", value.c_str());
       OperatingPeriodT1->publish_state(value.c_str());
       }   
    if(code == "C.8.2" ) {
       ESP_LOGD(TAG, "Operating Period T2: %s ", value.c_str());
       OperatingPeriodT2->publish_state(value.c_str());
       }   
    if(code == "C.82.0" ) {
       ESP_LOGD(TAG, "Operating Period Negative energy: %s ", value.c_str());
       OperatingPeriodTotalNegativeEnergy->publish_state(value.c_str());
       }   
    if(code == "F.F" ) {
       ESP_LOGD(TAG, "Error Code: %s ", value.c_str());
       ErrorStatus->publish_state(value.toInt());
       }
    if(code == "0.0.0" ) {
       ESP_LOGD(TAG, "CustomerNr: %s ", value.c_str());
       CustomerNr->publish_state(value.c_str());
       }
    if(code == "0.2.1" ) {
       ESP_LOGD(TAG, "SW version: %s ", value.c_str());
       SWVersion->publish_state(value.c_str());
       }
    if(code == "0.3.0" ) {
       ESP_LOGD(TAG, "Active energy meter constant: %s [imp/kWh]", value.c_str());
       ActiveEnergyConstant->publish_state(value.toFloat());
       }
    if(code == "1.8.0") { // Energy a in total: a = |+a|+|-a|
       ESP_LOGD(TAG, "Energy Total: %s ", value.c_str());
       TotalEnergy->publish_state(value.toFloat());
       }
    if(code == "1.8.1") { // Positive active energy (A+) T1 [kWh]
       ESP_LOGD(TAG, "Positive energy in T1: %s %s", value.c_str(), unit.c_str());
       PositiveEnergyT1->publish_state(value.toFloat());
       }
    if(code == "1.8.2") { // Positive active energy (A+) T2 [kWh]
       ESP_LOGD(TAG, "Positive energy in T2: %s %s", value.c_str(), unit.c_str());
       PositiveEnergyT2->publish_state(value.toFloat());
       }
    if(code == "2.8.0") { // Negative energy total
       ESP_LOGD(TAG, "Negative energy Total: %s %s", value.c_str(), unit.c_str());
       TotalNegativeEnergy->publish_state(value.toFloat());
       }
    if(code == "21.8.0") { // Positive active energy L1 [kWh]
       ESP_LOGD(TAG, "Positive energy L1: %s %s", value.c_str(), unit.c_str());
       TotalPositiveEnergyL1->publish_state(value.toFloat());
       }
    if(code == "22.8.0") { // Negative active energy L1 [kWh]
       ESP_LOGD(TAG, "Negative energy L1: %s %s", value.c_str(), unit.c_str());
       TotalNegativeEnergyL1->publish_state(value.toFloat());
       }
    if(code == "41.8.0") { // Positive active energy L2 [kWh]
       ESP_LOGD(TAG, "Positive energy L2: %s %s", value.c_str(), unit.c_str());
       TotalPositiveEnergyL2->publish_state(value.toFloat());
       }
    if(code == "42.8.0") { // Negative active energy L2 [kWh]
       ESP_LOGD(TAG, "Negative energy L2: %s %s", value.c_str(), unit.c_str());
       TotalNegativeEnergyL2->publish_state(value.toFloat());
       }
    if(code == "61.8.0") { // Positive active energy L3 [kWh]
       ESP_LOGD(TAG, "Positive energy L3: %s %s", value.c_str(), unit.c_str());
       TotalPositiveEnergyL3->publish_state(value.toFloat());
       }
    if(code == "62.8.0") { // Negative active energy L3 [kWh]
       ESP_LOGD(TAG, "Negative energy L3: %s %s", value.c_str(), unit.c_str());
       TotalNegativeEnergyL3->publish_state(value.toFloat());
       }
  }

  void update() override {
    String result;
    ESP_LOGD(TAG, "iec Update() ");
    result = sendcmd(IDCMD);
  }

};  // End class iec62056elmeter
