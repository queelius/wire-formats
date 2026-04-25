---
title: "RoaringBitmap"
date: 2025-12-07
draft: true
tags:
- C++
- data-structures
- compressed-bitmaps
- roaring
- polyalgorithm
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 12
math: true
description: "A hybrid compressed bitmap that picks the optimal sub-representation (array, bitmap, or run-length) per 64K-integer chunk based on density. No single prior dominates: Roaring commits to none and adapts per chunk."
linked_project:
- pfc
- wire-formats
---

(Draft in progress. See plan Task 21 for full prose.)
