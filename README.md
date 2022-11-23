# Arduino-Based Sprinker Controller

## Synopsis

This code creates an automation system for agricultural watering systems.  Whilst it is intended for use in domestic gardens (albeit on a large scale), it coul be used to control drip systems, micro-sprays or other related systems as well.

## Specifications

Number of outputs: 16 Relays

## Schedule Storage

Schedule storage is designed around the EEPROM inside the ATMega2560. (4K)  For simplicity, I have created 30-minute "slots", and since there are 7 days in a week, we need only a single byte to store a week's settings.

The basic struct is set into an array which has one element per output (16 in total) so every 30 minutes, we check each of the array elements and mask it against the day of the week (0 - 6).  The result tells us whether or not to turn on a specific relay.


