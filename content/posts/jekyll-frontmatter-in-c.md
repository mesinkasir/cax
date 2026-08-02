---
layout: detil.cax
title: Implementing Jekyll-Inspired Frontmatter Parser in Pure C
description: No libyaml, no dependencies. How we parse nested hero.title in 120 lines of C.
date: 2026-06-23
image: /img/cax-opening.webp
tags:
    - c
    - jekyll
    - parser
---

We built `yaml_parser.c` that handles `hero.title: Hello` into nested CAXObject. No external lib, just string split and pointer magic.

Full source at github.com/mesinkasir/cax - MIT licensed.