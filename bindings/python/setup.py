# setup.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Builds and packages the sushitrack Python distribution."""

from __future__ import annotations

from setuptools import setup

setup(
    name="sushitrack",
    version="1.0.0",
    description="Python bindings for SushiTrack real-time multi-object tracking engine",
    author="Mustafa Garip & Sushi Systems",
    author_email="mustafagarip@sushisystems.io",
    url="https://github.com/SushiSystems/SushiTrack",
    license="PolyForm-Noncommercial-1.0.0",
    py_modules=["sushitrack"],
    python_requires=">=3.11",
)
