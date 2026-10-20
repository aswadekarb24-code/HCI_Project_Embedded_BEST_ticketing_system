# HCI Values

["HCI Project Embedded Bus Ticket Prompt.md"](HCI%20Project%20Embedded%20Bus%20Ticket%20Prompt.md) names three frameworks to follow without expanding them: Shneiderman's rules, Norman's principles of design, and Nielsen's heuristics. This document spells each one out and points at the concrete, shipped feature that satisfies it, so the mapping from brief to implementation is checkable rather than asserted.

## Shneiderman's Eight Golden Rules

**1. Strive for consistency.** Every screen shares one header bar (title left, language switch top-right), one color language (teal/blue = primary action, grey = secondary/Back, green = success/start, orange = destination/end, red-orange = error), and one button shape (filled rectangle, white centered label) across `draw_route`/`draw_stops`/`draw_payment`/`draw_ticket`/`draw_help` in `src/ui_sdl.c`. The on-screen keypad reuses the exact same `button_f` primitive as every other control, not a separate widget style.

**2. Enable frequent users to use shortcuts.** The route-number and stop-name search (new on-screen keypad, `draw_keyboard`/`click_keyboard`) lets a rider who already knows their route code (e.g. "213") skip straight past browsing/scrolling 473 entries — the same shortcut-for-experts role a type-ahead field plays on a desktop app, adapted to touch-only input.

**3. Offer informative feedback.** Every tap has a visible consequence: buttons change screens immediately, the selected stop turns green (start) or orange (end), the query box echoes exactly what's been typed with a cursor (`"%s_"` in `draw_keyboard`), and `booking_fare`'s result is shown before the rider commits to paying.

**4. Design dialogs to yield closure.** The flow is a strict, visible sequence — route → stops → payment → ticket — each with its own screen and a clear terminal state (`SCREEN_TICKET` showing a completed, printed-looking ticket), not an open-ended form.

**5. Offer simple error handling.** `booking_validate`/`booking_complete` (`src/booking.c`) reject an incomplete or same-start/end booking *before* money changes hands, and a printer-out-of-paper condition (`BOOKING_PRINTER_EMPTY`) surfaces as a plain-language, translated notice (`T_PRINTER_EMPTY`) rather than a crash or a silently-lost ticket.

**6. Permit easy reversal of actions.** A `Back` button exists on every screen except the initial route list (where there is nothing to go back to), and selecting a different start/end stop, or a different payment method, is a single tap with no confirmation gauntlet. Opening the search keypad never commits a filter irreversibly — `Clear` and further edits remain available until `Done` is tapped, and the underlying list is never destroyed, only filtered.

**7. Support internal locus of control.** The rider drives every transition (tap a route, tap stops, tap pay) — the system never auto-advances or times out mid-flow, consistent with a public kiosk where the person, not the machine, should feel in charge of the transaction.

**8. Reduce short-term memory load.** The chosen route's number and name stay printed at the top of the stop screen (`draw_stops`'s header line) so the rider never has to remember what they picked two screens back; the map keeps every real stop visible as a small circle for spatial recognition instead of requiring the rider to recall stop names from memory.

## Norman's Principles of Design

**Affordances & signifiers.** Buttons look pressable (filled, high-contrast rectangles); the search field looks like a text field (white-on-dark label area with a trailing cursor glyph); the `+`/`-` map controls look exactly like what they are, not icons requiring interpretation.

**Visibility.** Nothing needed to operate the kiosk is hidden in a menu: route list, map, zoom controls, search, language switch, and help are all on-screen simultaneously (within their screen's context), not nested behind a settings icon.

**Feedback.** Covered concretely under Shneiderman rule 3 above — every state change (selection, zoom level, scroll position, search query, payment method, printer error) has an immediate, visible on-screen response.

**Mapping.** The map and the stop list are two views of the *same* selection state: tapping a stop on the satellite map and tapping it in the sidebar list do the same thing (`stop_screen_hit` vs. the list's `stop_row_box` hit-test both feed the same `a->booking.start_stop`/`end_stop`), so the rider's mental model ("this dot = this list row") holds.

**Constraints.** The UI prevents invalid states structurally rather than by validation-after-the-fact: `Continue` is only reachable once both a start and end stop are chosen and they differ (`booking_validate`); the zoom camera is clamped so it can never pan past the edge of the basemap (`camera_viewport`'s bounds-clamping in `src/mapview.c`); the search keypad only offers Latin letters/digits, which is exactly the alphabet route codes and the underlying English stop data use, so there's no way to type a query that could never match anything by construction.

**Conceptual model.** The whole app mirrors how a rider already thinks about a real bus trip — pick a route, pick where you get on and off, pay, get a ticket — rather than exposing any internal data structure (GTFS trip IDs, pixel coordinates, language enums) to the person using it.

## Nielsen's Ten Usability Heuristics

**1. Visibility of system status.** The booking/payment flow always shows which screen you're on via the header title; `camera` zoom level and scroll position are reflected live (scroll arrows grey out at the start/end of a list, exactly like a disabled button should).

**2. Match between system and the real world.** Real BEST route numbers, real stop names, and a real satellite view of Mumbai (`tools/import_gtfs.py`, `tools/fetch_basemap.py`) replace what used to be 4 invented routes on an AI-generated mock map — the system now matches the actual world the rider is navigating, not an abstraction of it.

**3. User control and freedom.** Back buttons and a persistent, editable search query (not a one-shot filter) mean a rider can always retreat from or revise a choice; nothing is a one-way door except completing payment.

**4. Consistency and standards.** One shared `Box`/`button`/`text` vocabulary across the whole file; green always means "start/success", orange always means "destination", grey always means "neutral/secondary/back" — colors are never reused with a different meaning on a different screen.

**5. Error prevention.** `MAX_ROUTES`/`MAX_STOPS` bounds plus `tools/generate_catalog.py`'s validation prevent a malformed catalogue from ever reaching the firmware; `booking_validate` prevents a same-stop or incomplete booking from ever reaching the payment screen in the first place, which is stronger than catching the error after the fact.

**6. Recognition rather than recall.** The satellite map plus always-visible stop dots let a rider *recognize* their stop by its real-world location instead of having to *recall* its exact printed name; the scrollable list is always paired with the same visual map, so either memory style (place vs. name) works.

**7. Flexibility and efficiency of use.** The search keypad (new) is the direct answer to this heuristic: a novice can browse/scroll the full list, while someone who already knows "213" or "Churchgate" can search straight to it — the same two-speed interaction Nielsen's rule asks for, without adding a mode switch the novice path has to route around.

**8. Aesthetic and minimalist design.** The map never renders all 473 routes' colored lines simultaneously — only the always-present stop dots plus whichever single route is currently selected — a deliberate choice (see ARCHITECTURE.md) to keep the screen legible at real-world data scale instead of letting "more real data" become visual noise.

**9. Help users recognize, diagnose, and recover from errors.** The printer-empty notice is plain language, translated into the rider's chosen language, placed right above the action that triggered it (`draw_payment`'s notice banner) — not an error code or a stack trace.

**10. Help and documentation.** A dedicated, one-tap-away Help screen (`T_HELP`/`draw_help`) carries the helpline number, an emergency number, and the data/imagery attributions — reachable from the route screen without derailing an in-progress booking.

## Multilingual access as an HCI requirement, not just a feature

The original prompt lists "Multilingual support through a sidebar toggle" and "Ease of use by lesser educated people" as features in their own right, which is really Shneiderman/Nielsen consistency and recognition applied across *languages*, not just within one. English/Marathi/Hindi/Gujarati each get a dedicated, bundled font (`assets/fonts/`) rather than relying on whatever happens to be installed on the host, specifically because inconsistent rendering (text silently turning into empty boxes) is itself a usability failure — a rider who can't read the UI at all has had every other heuristic above revoked for them. See SETUP.md's font-coverage notes for the concrete bugs this fixed.
