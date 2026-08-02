---
layout: detil.cax
title: Memory Management Tricks for 0.4s Builds
description: How CAX uses arena allocation and string pooling to build sites faster than Hugo and Eleventy.
date: 2026-06-23
image: /img/cssg.webp
tags:
    - c
    - memory
    - optimization
---

We don't use libyaml. We parse frontmatter with custom arena allocator. One malloc per file, free after build. Zero leak.

This is the same technique I use for clients on Fiverr to optimize their legacy C apps.