# Off-Grid 1kW Inverter (24V DC -> 230V AC)

Projekt przetwornicy off-grid o mocy 1 kW przeznaczonej do konwersji napięcia z instalacji 24V DC na napięcie sieciowe 230V AC 50Hz.
Urządzenie składa się z dwóch głównych stopni konwersji energii:

1. Przetwornica DC/DC podwyższająca napięcie z 24V do około 325V DC
2. Falownik DC/AC generujący napięcie 230V AC 50Hz

System sterowany jest przez mikrokontroler odpowiedzialny za generację sygnałów sterujących, pomiary oraz zabezpieczenia.

---

# Główne parametry

| Parametr           | Wartość        |
| ------------------ | -------------- |
| Moc znamionowa     | 1000W          |
| Napięcie wejściowe | 24V DC         |
| Napięcie pośrednie | ~325V DC       |
| Napięcie wyjściowe | 230V AC        |
| Częstotliwość      | 50Hz           |
| Typ pracy          | Off-grid       |
| Sterowanie         | Mikrokontroler |

---

# Architektura systemu

System składa się z kilku głównych bloków funkcjonalnych:

24V DC
↓
DC/DC Boost Converter
↓
325V DC Bus
↓
Full Bridge Inverter
↓
LC Filter
↓
230V AC 50Hz

Dodatkowo system zawiera:

* sterowniki bramek tranzystorów mocy
* układy pomiarowe prądu i napięcia
* system chłodzenia

---

# Moduły projektu

## 1. Przetwornica DC/DC (24V -> 325V)

Odpowiada za podwyższenie napięcia z poziomu akumulatora do napięcia magistrali DC.

Główne elementy:

* transformator wysokiej częstotliwości
* tranzystory MOSFET
* sterowniki bramek

---

## 2. Falownik (325V DC -> 230V AC)

Falownik generuje napięcie sinusoidalne poprzez modulację PWM.

Topologia:

Full Bridge (H-Bridge)

Sterowanie:

SPWM (Sinusoidal Pulse Width Modulation)

Elementy:

* tranzystory MOSFET / IGBT
* drivery bramek
* filtr LC

---

## 3. Mikrokontroler

Mikrokontroler jest centralną jednostką sterującą systemem.

Zadania:

* generowanie sygnałów PWM
* sterowanie przetwornicą DC/DC
* sterowanie falownikiem
* pomiar napięcia i prądu
* implementacja zabezpieczeń
* diagnostyka systemu

---

## 4. Sterowniki elementów mocy

Sterowniki bramek odpowiadają za szybkie i bezpieczne przełączanie tranzystorów.

Wymagania:

* izolacja galwaniczna
* odpowiedni prąd sterowania bramki
* kontrola dead-time
* ochrona przed zwarciem

---

## 5. Pomiar prądu i napięcia

Układ pomiarowy pozwala na monitorowanie parametrów pracy systemu.

Pomiar obejmuje:

* napięcie wejściowe
* napięcie magistrali DC
* napięcie wyjściowe AC
* prąd wyjściowy

Stosowane elementy:

* rezystory pomiarowe
* czujniki Halla
* dzielniki napięcia

---

## 6. Konstrukcja mechaniczna

Projekt mechaniczny obejmuje:

* obudowę
* system chłodzenia
* rozmieszczenie PCB
* radiatory dla elementów mocy

---

# Struktura repozytorium

```
hardware/ 
    dcdc_converter 
    drivers 
    inverter 
    measurements
```

---

# Etapy projektu

1. Projekt architektury systemu
2. Projekt przetwornicy DC/DC
3. Projekt falownika
4. Implementacja firmware mikrokontrolera
5. Projekt PCB

---

# Licencja

Projekt open hardware / open source.
