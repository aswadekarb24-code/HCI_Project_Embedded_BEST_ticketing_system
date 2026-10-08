# BEST Mumbai Embedded Ticket Kiosk

A graphical, click-as-touch ticket-booking kiosk prototype for BEST Mumbai. The application uses embedded C for its route, booking, localization, payment, and printer logic; SDL2 emulates the display during development.

Features include:

- An offline satellite basemap of Mumbai with discrete, button-driven zoom and pan.
- The real BEST route/stop catalogue (473 routes, thousands of stops) sourced from the [`croyla/mumbai-gtfs`](https://github.com/croyla/mumbai-gtfs) feed, with map and scrollable-list stop selection -- stops drawn as circles joined by a line, with the selected route highlighted.
- An on-screen search keypad to jump straight to a route number or stop name instead of scrolling.
- A four-language UI: English, Marathi, Hindi, and Gujarati, each with its own bundled font so text renders correctly regardless of what's installed on the host.
- Payment choice, fare calculation, a 90-minute ticket with a scannable QR code, PDF save/print actions, printer-paper error handling, and helpline access.

Generated tickets are saved in the operating system's BEST TicketKiosk application data folder. The QR contains the ticket reference and is intended for prototype scanning; validating it requires a ticketing backend.

Prototype fare is ₹10 plus ₹5 for each stop-index interval between the selected stops. This intentionally simple rule is for the demo and does not represent BEST's official fare calculation.

Route/stop *names* come from the GTFS feed (English only) and are auto-transliterated into Devanagari (Marathi/Hindi) and Gujarati script; this is a best-effort phonetic rendering of proper nouns, not a human-reviewed translation. All fixed UI text (buttons, screen titles, errors) is fully hand-translated in all four languages.

`assets/mumbai_satellite.png` is a Sentinel-2 cloudless satellite mosaic. Imagery © EOX IT Services GmbH (s2maps.eu), licensed CC BY-NC-SA 4.0. Route/stop data © `croyla/mumbai-gtfs` contributors, licensed MIT-0. Neither is accurate enough for real-world navigation.

See [SETUP.md](SETUP.md), [ARCHITECTURE.md](ARCHITECTURE.md), [TECH_STACK.md](TECH_STACK.md), and [HCI_Values.md](HCI_Values.md).
