---
layout: detil.cax
title: Creating Collection Controllers Like Jekyll in C
description: How content/posts.md acts as controller for your entire collection - pagination, hero, layout in one file.
date: 2026-06-23
image: /img/cssg.webp
tags:
    - jekyll
    - collections
    - c
---

In CAX, `content/minimal.md` with `collection: minimal` controls all posts in `content/minimal/`. Hero, pagination, layout - all in one place. No _config.yml mess.

This is the Jekyll DX we love, but 30x faster because it's pure C.