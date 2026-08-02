# CAX SSG - Static Site Generator in C

A Starter Project CAX SSG

![C Static Site Generator](cax-starter.jpg)

**CAX SSG** is a high-performance, zero-dependency static site generator written in pure C. Inspired by the developer experience of Eleventy and Jekyll, but engineered for raw C speed.

Read Documentation [https://cax.axcora.com](https://cax.axcora.com)

Run Demo Project [https://cax.axcora.com/starter/](https://cax.axcora.com/starter/)

> **C for Speed. Built by AXCORA.**

![C](https://img.shields.io/badge/C-99-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![Speed](https://img.shields.io/badge/build-%3C50ms-orange)


---

## Support & Donate

If you like this project, please support:

- PayPal: https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=JVZVXBC4N9DAN
- Gumroad Coffee: https://creativitaz.gumroad.com/coffee
- GitHub Sponsors: https://github.com/sponsors/mesinkasir

---

## Tech

### Core
- **Pure C99** - No Node.js, Ruby, Python. Single binary.
- **Zero Dependencies** - GCC + Make only.
- **Blazing Fast** - Build 100 pages in < 50ms, < 5MB RAM.
- **Markdown + Frontmatter** - YAML frontmatter support.

### Features
- **Pagination Controllers** - Any `content/*.md` with `collection:` + `pagination:` becomes a list page.
- **Dynamic Collections** - Every folder in `content/` is a collection.
- **Multi Layout Support** - `posts-list.cax`, `services-list.cax`, `portfolio-grid.cax` - fully dynamic.
- **Advanced Template Engine** - `{{ variables }}`, `{% for %}`, `{% if %}`, `{% include %}`.
- **Pagination Object** - `pagination.items`, `pagination.prev_url`, `pagination.next_url`, `pagination.current_page`, `pagination.total_pages`, `pagination.total_items`.

### SEO & Production Ready
- **Tags & Taxonomies** - Automatic `/tags/{tag}/` pages.
- **Sitemap** - Full `sitemap.xml` including pagination pages.
- **Feeds** - `feed.xml`, `rss.xml`, `feed.json` (JSON Feed 1.1).
- **Robots** - Auto `robots.txt` with sitemap reference.
- **Built-in Server** - `cax start` serves `site/` at `http://localhost:8080`.

---

## Installation

### Prerequisites
- GCC Compiler
- Make (or MinGW32-make on Windows)

### Build from Source

```bash
# Clone
git clone https://github.com/mesinkasir/cax.git
cd cax

# Windows
mingw32-make clean
mingw32-make

# Linux / macOS
make clean
make linux
```

Output: `cax.exe` (Windows) or `cax` (Linux/macOS).

---

## Quick Start

```bash
cax init          # Initialize new project
cax build         # Build to site/
cax start         # Build + serve at http://localhost:8080
```

---

## Project Structure

```text
C:.
├── content/
│   ├── index.md                 # -> /index.html
│   ├── about.md                 # -> /about/index.html
│   ├── posts.md                 # Controller for posts collection
│   ├── services.md              # Controller for services collection
│   ├── posts/                   # Collection: posts
│   │   ├── why-c.md
│   │   └── template-engine.md
│   ├── products/                # Collection: products
│   └── services/                # Collection: services
├── templates/
│   ├── layouts/
│   │   ├── default.cax
│   │   ├── home.cax
│   │   ├── posts-list.cax       # List layout for posts
│   │   ├── services-list.cax    # List layout for services
│   │   ├── steampunk.cax
│   │   └── tag.cax
│   └── partials/
│       ├── header.cax
│       ├── footer.cax
│       └── index/
├── _data/
│   ├── metadata.json            # site.title, site.url, etc
│   ├── nav.json
│   └── config.json
├── public/
│   ├── css/style.css
│   └── img/
├── src/                         # C source
├── include/cax.h
├── site/                        # Generated output
│   ├── index.html
│   ├── sitemap.xml
│   ├── robots.txt
│   ├── feed.xml
│   ├── rss.xml
│   ├── feed.json
│   ├── posts/
│   │   ├── index.html
│   │   └── page/2/index.html
│   └── tags/
├── Makefile
└── cax.exe
```

---

## Content Management

### 1. Basic Page

`content/about.md`:

```markdown
---
title: About Us
description: Learn about CAX
layout: default.cax
---

This is about page.
```

### 2. Eleventy-Style Collection Controller

This is the core of CAX V2. Create a file `content/posts.md` to control the `/posts/` listing.

**content/posts.md:**

```markdown
---
layout: posts-list.cax
title: Blog - CAX SSG
description: All articles about CAX
collection: posts
pagination: 6
---

<p class="lead">Welcome to my blog. Here are all my articles.</p>
```

**What it does:**

- `collection: posts` - Lists all markdown files from `content/posts/`
- `pagination: 6` - 6 items per page
- `layout: posts-list.cax` - Uses this template
- Output: `/posts/`, `/posts/page/2/`, `/posts/page/3/`

**Another collection - content/services.md:**

```yaml
---
layout: services-list.cax
title: Our Services
collection: services
pagination: 9
---
```

### 3. Collection Item

`content/posts/why-c.md`:

```markdown
---
title: Why C for SSG?
description: Performance benchmark
layout: post.cax
tags:
  - c
  - performance
---

Content here...
```

---

## Templating

CAX uses `.cax` templates, compatible with Liquid-style syntax.

### Variables

```html
<title>{{ title }} - {{ site.title }}</title>
<meta name="description" content="{{ description }}">
{{ content }}
```

### Includes

```html
{% include header.cax %}
{% include footer.cax %}
{% include index/hero.cax %}
```

### Loops - Eleventy Style

**posts-list.cax:**

```html
<div class="row">
  {% for post in pagination.items %}
  <div class="col-md-4 mb-4">
    <div class="card">
      <div class="card-body">
        <h5 class="card-title">
          <a href="{{ post.url }}">{{ post.title }}</a>
        </h5>
        <p class="card-text">{{ post.description }}</p>
      </div>
    </div>
  </div>
  {% endfor %}
</div>

<nav class="d-flex justify-content-between">
  <div>
    {% if pagination.prev_url %}
    <a href="{{ pagination.prev_url }}" class="btn btn-primary">← Prev</a>
    {% endif %}
  </div>
  <div>
    {% if pagination.next_url %}
    <a href="{{ pagination.next_url }}" class="btn btn-primary">Next →</a>
    {% endif %}
  </div>
</nav>
```

**Available Pagination Object:**

| Variable | Description |
|----------|-------------|
| `pagination.items` | Array of posts for current page |
| `pagination.current_page` | Current page number |
| `pagination.total_pages` | Total pages |
| `pagination.total_items` | Total items |
| `pagination.prev_url` | Previous page URL |
| `pagination.next_url` | Next page URL |

### Conditionals

```html
{% if pagination.prev_url %}
  <a href="{{ pagination.prev_url }}">Previous</a>
{% endif %}
```

---

## Data Files

`_data/metadata.json`:

```json
{
  "site": {
    "title": "CAX Enterprise SSG",
    "description": "High performance Static Site Generator in C",
    "url": "https://cax.axcora.com",
    "author": "AXCORA"
  }
}
```

`_data/nav.json`:

```json
{
  "nav1_name": "Home",
  "nav1_url": "/",
  "nav2_name": "Posts",
  "nav2_url": "/posts/",
  "nav3_name": "About",
  "nav3_url": "/about/"
}
```

Access in templates: `{{ site.title }}`, `{{ nav1_name }}`

---

## SEO & Feeds

Auto-generated in `site/` on every build:

- `sitemap.xml` - All pages + collection pages + pagination pages
- `robots.txt` - Allow all + Sitemap reference
- `feed.xml` - RSS 2.0
- `rss.xml` - RSS 2.0 alias
- `feed.json` - JSON Feed 1.1

Verify after build:

```
http://localhost:8080/sitemap.xml
http://localhost:8080/robots.txt
http://localhost:8080/feed.xml
http://localhost:8080/feed.json
```

---

## Commands Reference

| Command | Description |
|---------|-------------|
| `cax init` | Initialize new project structure |
| `cax build` | Build static site to `site/` |
| `cax start` | Build and start dev server at `:8080` |
| `make clean` / `mingw32-make clean` | Clean object files |

---

## Performance

| Metric | Value |
|--------|-------|
| Build Time (100 pages) | < 50ms |
| Binary Size | < 500KB |
| Memory Usage | < 5MB |
| Dependencies | Zero |

Written in pure C with custom Markdown parser, YAML frontmatter parser, and template engine.

---

## Platform Support

**Windows:**

```bash
mingw32-make
./cax.exe build
./cax.exe start
```

**Linux / macOS:**

```bash
make linux
./cax build
./cax start
```

---

## Author

**AXCORA**

- Website: https://www.axcora.com
- GitHub: https://github.com/mesinkasir

---

## Support & Donate

If you like this project, please support:

- PayPal: https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=JVZVXBC4N9DAN
- Gumroad Coffee: https://creativitaz.gumroad.com/coffee
- GitHub Sponsors: https://github.com/sponsors/mesinkasir

---
## Documentation

Read Documentation [https://cax.axcora.com](https://cax.axcora.com)

Run Demo Project [https://cax.axcora.com/starter/](https://cax.axcora.com/starter/)


---

**CAX SSG - Eleventy DX with C Speed.**
