# HCI Project : Embedded Bus Ticket Booker for BEST Mumbai

## Introduction

We aim to build a Embedded systems Application, that allows users to

1. Have a clean look at the Map of Mumbai, with color coded bus routes depending on regions
2. Select a bus route with ease.
3. Able to select start and end stops through both map interaction and manual check box selection.
4. Select Appropriate payment system, and proceed with payment steps.
5. Print the Bus ticket.

## Features

1. Multilingual support through a sidebar toggle.
2. Ease of use by lesser educated people.
3. Detection of availablity of tickets to print. (appropriate error handling)
4. Helpline numbers on support

## Coding practices

1. Object oriented Programming
2. Follow SOLID Principles and DRY.
3. Make addition/modification of stuff easier (such as adding a new bus route, editing an existing one, deletion, or adding new language, or changing the goverment seal to be used in ticket).
4. Make sure to add proper unit tests.
5. Create bash scripts for easy build/test runs.

## HCI parts

Follow:
- Schneiderman's rules
- Normans principles of design
- Nielsens heuristics

## Tech stack.

Use embedded C for majority of the workm with python mainly for config.

## Emulation

Design the code to be as realistic to real world architecture as possible.  Select appropriate QEMU based emulator

## Documentation

Store the summary of the project, its features and use in README.md.
Create SETUP.md for giving guidelines to developers for installation and testing/modification of the code.
Give instructions wrt to the following Operating Systems:
- Windows
- MacOS
- Arch Linux
- Ubuntu 
- Fedora Linux
Store the diagrams and tech stack with their explanations in ARCHITECTURE.md and TECH_STACK.md respectively. 
