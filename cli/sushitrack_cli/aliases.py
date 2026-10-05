# aliases.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Holds the old spellings st still answers to, each with its last day."""

from __future__ import annotations

from datetime import date

from sushicore.aliases import Alias, AliasTable

from .config import PROFILE

#: The last day every transitional spelling works.
K_REMOVED_AFTER = date(2027, 3, 25)

#: The table st registers its old command words through.
ALIASES = AliasTable(PROFILE.program, (
    Alias("st docker build", "st container build", K_REMOVED_AFTER),
    Alias("st docker run", "st container run", K_REMOVED_AFTER),
))
