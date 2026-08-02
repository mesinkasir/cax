#include "builder.h"
#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include "../data/data_manager.h"
#include "../data/yaml_parser.h"
#include "../data/json_parser.h"
#include "../frontmatter/frontmatter.h"
#include "../markdown/markdown.h"
#include "../template/template_engine.h"
#include "../sitemap/sitemap.h"
#include "../collection/collection.h"
#include "../pagination/pagination.h"
#include "../tags/tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

static char *clean_trim(char *str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}
static int file_exists(const char *path){
    FILE *f=fopen(path,"r");
    if(f){ fclose(f); return 1; }
    return 0;
}
static void get_coll_list_template(const char *coll_name, char *out, size_t out_size){
    snprintf(out, out_size, "templates/layouts/%s.cax", coll_name);
    if(file_exists(out)) return;
    snprintf(out, out_size, "templates/layouts/%s-list.cax", coll_name);
    if(file_exists(out)) return;
    snprintf(out, out_size, "default.cax");
}
static time_t get_mtime(const char *path){
    struct stat st;
    if(stat(path,&st)==0) return st.st_mtime;
    return 0;
}
static time_t scan_dir(const char *dir){
    time_t max=0;
    DIR *d=opendir(dir);
    if(!d) return get_mtime(dir);
    struct dirent *e;
    while((e=readdir(d))!=NULL){
        if(e->d_name[0]=='.') continue;
        char p[1024]; snprintf(p,sizeof(p),"%s/%s",dir,e->d_name);
        time_t t;
        if(cax_is_directory(p)) t=scan_dir(p);
        else t=get_mtime(p);
        if(t>max) max=t;
    }
    closedir(d);
    return max;
}
static void inject_livereload_script(){
    cax_write_file("site/__cax_live.js","(function(){let h=null;setInterval(async()=>{try{let t=await fetch(location.href,{cache:'no-store'}).then(r=>r.text());if(h===null)h=t.length;else if(h!=t.length)location.reload();}catch(e){}},1000);})();");
#ifdef _WIN32
    system("powershell -Command \"$files=Get-ChildItem site -Filter *.html -Recurse -ErrorAction SilentlyContinue; foreach($f in $files){ $c=Get-Content $f.FullName -Raw -ErrorAction SilentlyContinue; if($c -and $c -notmatch '__cax_live'){ $c=$c -replace '</body>','<script src=\\\"/__cax_live.js\\\"></script></body>'; Set-Content $f.FullName $c -NoNewline } }\" >nul 2>&1");
#else
    system("find site -name '*.html' -exec sed -i 's|</body>|<script src=/__cax_live.js></script></body>|g' {} \\; 2>/dev/null");
#endif
}
static void remove_livereload_script(){
    remove("site/__cax_live.js");
}
static void minify_and_copy_assets(void) {
    cax_copy_directory("public", "site");
    char css_path[4096];
    snprintf(css_path, sizeof(css_path), "site/css/style.css");
    char *css_content = cax_read_file(css_path);
    if (css_content) {
        size_t len = strlen(css_content);
        char *minified = malloc(len + 1);
        if (minified) {
            size_t j = 0; int in_space = 0; int in_quote = 0; char quote_char = '\0';
            for (size_t i = 0; i < len; i++) {
                char c = css_content[i];
                if ((c == '\'' || c == '"') && (i == 0 || css_content[i-1]!= '\\')) {
                    if (!in_quote) { in_quote = 1; quote_char = c; }
                    else if (quote_char == c) { in_quote = 0; }
                }
                if (!in_quote) {
                    if (c == '\n' || c == '\r' || c == '\t') c = ' ';
                    if (c == ' ') { if (!in_space) { minified[j++] = c; in_space = 1; } continue; }
                }
                minified[j++] = c; in_space = 0;
            }
            minified[j] = '\0';
            cax_write_file(css_path, minified);
            free(minified);
        }
        free(css_content);
    }
}
void cax_builder_init_project(void) {
    cax_create_directory("content/posts");
    cax_create_directory("content/services");
    cax_create_directory("_data");
    cax_create_directory("templates/layouts");
    cax_create_directory("templates/partials");
    cax_create_directory("public/css");
    cax_create_directory("site");
    cax_write_file("_data/metadata.json", "{\n \"site\": {\n \"title\": \"CAX Enterprise SSG\",\n \"description\": \"High performance Static Site Generator in C\",\n \"url\": \"http://localhost:8080\",\n \"author\": \"Axcora\"\n }\n}\n");
    cax_write_file("_data/nav.json","{\n \"nav1_name\": \"Home\",\n \"nav1_url\": \"/\",\n \"nav2_name\": \"Posts\",\n \"nav2_url\": \"/posts/\",\n \"nav3_name\": \"About\",\n \"nav3_url\": \"/about/\"\n}\n");
    cax_write_file("templates/partials/header.cax","<header class=\"py-4 border-bottom bg-white shadow-sm mb-4\">\n <div class=\"container d-flex justify-content-between align-items-center\">\n <a class=\"navbar-brand fw-bold text-dark text-decoration-none fs-4\" href=\"/\">{{ title }}</a>\n <nav class=\"nav\">\n <a class=\"nav-link text-secondary fw-medium px-3\" href=\"{{ nav1_url }}\">{{ nav1_name }}</a>\n <a class=\"nav-link text-secondary fw-medium px-3\" href=\"{{ nav2_url }}\">{{ nav2_name }}</a>\n <a class=\"nav-link text-secondary fw-medium px-3\" href=\"{{ nav3_url }}\">{{ nav3_name }}</a>\n </nav>\n </div>\n</header>\n");
    cax_write_file("templates/partials/footer.cax","<footer class=\"text-center py-4 text-muted\">\n <small>&copy; 2026 {{ author }}. Built with CAX SSG.</small>\n</footer>\n");
    cax_write_file("templates/layouts/default.cax","<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n <meta charset=\"UTF-8\">\n <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n <title>{{ title }}</title>\n <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/css/bootstrap.min.css\" rel=\"stylesheet\">\n <link href=\"/css/style.css\" rel=\"stylesheet\">\n</head>\n<body class=\"bg-light\">\n {% include header.cax %}\n <main class=\"container my-5\">\n <div class=\"card shadow-sm border-0 p-4\">\n <h1 class=\"mb-3 text-primary\">{{ title }}</h1>\n <p class=\"text-muted\">{{ description }}</p>\n <div class=\"mb-3\">{{ tags_html }}</div>\n <hr>\n <div class=\"content-body text-secondary lh-lg\">\n {{ content }}\n </div>\n <div class=\"mt-4 pt-3 border-top d-flex justify-content-between\">\n <div>{{ prev_link }}</div>\n <div>{{ next_link }}</div>\n </div>\n </main>\n {% include footer.cax %}\n</body>\n</html>\n");
    cax_write_file("templates/layouts/home.cax","<!DOCTYPE html>\n<html lang=\"id\">\n<head>\n <meta charset=\"UTF-8\">\n <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n <title>{{ title }}</title>\n <link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/axcora-css@1.0.1/axcora.min.css\">\n <link href=\"/css/style.css\" rel=\"stylesheet\">\n</head>\n<body>\n{% include header.cax %}\n<div class=\"container\">\n <h4>Centered Content</h4>\n <img src=\"{{ image }}\" alt=\"{{ hero_title }}\" class=\"img-fluid\">\n <p>{{ hero_description }}</p>\n</div>\n<main class=\"container\">\n{{ content }}\n</main>\n{% include footer.cax %}\n</body>\n</html>\n");
    cax_write_file("public/css/style.css","body {\n font-family: system-ui, -apple-system, sans-serif;\n background-color: #f8f9fa;\n}\n");
    cax_write_file("content/index.md","---\nlayout: home.cax\ntitle: Welcome to CAX Engine\ndescription: Blazing fast static site generator written in pure C.\n---\n<h3>Hello World from C Architecture</h3>\n<p>End-to-end integration verified successfully.</p>\n");
    cax_write_file("content/about.md","---\ntitle: About Us\ndescription: Learn more about CAX Static Site Generator architecture.\nlayout: default.cax\ntags:\n - about\n - cax\n---\n<p>This page is automatically rendered directly from a root content markdown file.</p>\n");
    cax_write_file("content/posts/first-post.md","---\ntitle: First Blog Post in C\ndescription: Exploring static site generation speed.\nlayout: default.cax\ntags:\n - c\n - blog\n---\n<p>This is the first article inside our automated collection loop.</p>\n");
    cax_write_file("content/posts/second-post.md","---\ntitle: Advanced Memory Management\ndescription: Fast string parsing and allocation.\nlayout: default.cax\ntags:\n - c\n - memory\n---\n<p>This is the second article with next and previous navigation links.</p>\n");
}
static CAXObject *copy_metadata_object(CAXObject *src) {
    if (!src) return NULL;
    CAXObject *dst = cax_object_new();
    for (int i = 0; i < src->count; i++) {
        if (src->fields[i].type == CAX_TYPE_STRING) cax_object_set_string(dst, src->fields[i].key, src->fields[i].string_val);
        else if (src->fields[i].type == CAX_TYPE_POINTER) cax_object_set_pointer(dst, src->fields[i].key, src->fields[i].pointer_val);
    }
    return dst;
}
static char *build_tags_html(CAXArray *tags_arr) {
    if (!tags_arr || tags_arr->count == 0) return cax_strdup("");
    size_t buf_size = 8192;
    char *buf = malloc(buf_size);
    if (!buf) return cax_strdup("");
    buf[0] = '\0';
    snprintf(buf, buf_size, "<div class=\"tags-container\">");
    for (int i = 0; i < tags_arr->count; i++) {
        const char *tag_str = (const char *)tags_arr->items[i];
        if (tag_str) {
            char badge[512];
            snprintf(badge, sizeof(badge), "<a href=\"/tags/%s/\" class=\"badge bg-secondary text-decoration-none me-1\">%s</a>", tag_str, tag_str);
            if (strlen(buf) + strlen(badge) + 32 < buf_size) strcat(buf, badge);
        }
    }
    strcat(buf, "</div>");
    return buf;
}
static void process_collection_dir(const char *dir_path, const char *coll_name, CAXArray *master_array) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir))!= NULL) {
        if (entry->d_name[0] == '.') continue;
        char full_path[8192];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (cax_is_directory(full_path)) continue;
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".md")!= 0) continue;
        char *file_content = cax_read_file(full_path);
        if (!file_content) continue;
        CAXParsedDocument *doc = cax_frontmatter_parse(file_content);
        free(file_content);
        if (!doc) continue;
        CAXObject *item_obj = cax_object_new();
        if (doc->metadata) {
            for (int i = 0; i < doc->metadata->count; i++) {
                if (doc->metadata->fields[i].type == CAX_TYPE_STRING)
                    cax_object_set_string(item_obj, doc->metadata->fields[i].key, doc->metadata->fields[i].string_val);
                else if (doc->metadata->fields[i].type == CAX_TYPE_POINTER)
                    cax_object_set_pointer(item_obj, doc->metadata->fields[i].key, doc->metadata->fields[i].pointer_val);
            }
        }
        char slug[1024];
        strncpy(slug, entry->d_name, sizeof(slug)-1);
        slug[sizeof(slug)-1] = '\0';
        char *dot = strrchr(slug, '.'); if (dot) *dot = '\0';
        char url_buf[1024];
        snprintf(url_buf, sizeof(url_buf), "/%s/%s/", coll_name, slug);
        cax_object_set_string(item_obj, "url", url_buf);
        cax_object_set_string(item_obj, "permalink", url_buf);
        cax_object_set_string(item_obj, "slug", slug);
        cax_array_add(master_array, item_obj);
        cax_frontmatter_free(doc);
    }
    closedir(dir);
}
static void copy_all_data_to_context(CAXObject *src, CAXObject *dst) {
    if (!src ||!dst) return;
    for (int i = 0; i < src->count; i++) {
        if (src->fields[i].type == CAX_TYPE_STRING) cax_object_set_string(dst, src->fields[i].key, src->fields[i].string_val);
        else if (src->fields[i].type == CAX_TYPE_POINTER) cax_object_set_pointer(dst, src->fields[i].key, src->fields[i].pointer_val);
    }
}
static void flatten_site_from_metadata(CAXObject *src, CAXObject *dst) {
    if (!src ||!dst) return;
    CAXObject *metadata = cax_object_get_pointer(src, "metadata");
    if (metadata) {
        CAXObject *site = cax_object_get_pointer(metadata, "site");
        if (site) {
            for (int i = 0; i < site->count; i++) {
                if (site->fields[i].type == CAX_TYPE_STRING) cax_object_set_string(dst, site->fields[i].key, site->fields[i].string_val);
            }
        }
    }
}
static void render_collection_items(const char *dir_path, const char *coll_name, CAXArray *master_array, CAXObject *global_data) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir))!= NULL) {
        if (entry->d_name[0] == '.') continue;
        char full_path[8192];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (cax_is_directory(full_path)) continue;
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".md")!= 0) continue;
        char *file_content = cax_read_file(full_path);
        if (!file_content) continue;
        CAXParsedDocument *doc = cax_frontmatter_parse(file_content);
        free(file_content);
        if (!doc) continue;
        CAXObject *context = cax_object_new();
        copy_all_data_to_context(global_data, context);
        flatten_site_from_metadata(global_data, context);
        CAXObject *nav_obj = cax_object_get_pointer(global_data, "nav");
        if (nav_obj) {
            cax_object_set_pointer(context, "nav", nav_obj);
            for (int i = 0; i < nav_obj->count; i++) if (nav_obj->fields[i].type == CAX_TYPE_STRING) cax_object_set_string(context, nav_obj->fields[i].key, nav_obj->fields[i].string_val);
        }
        if (doc->metadata) {
            for (int i = 0; i < doc->metadata->count; i++) {
                if (doc->metadata->fields[i].type == CAX_TYPE_STRING) cax_object_set_string(context, doc->metadata->fields[i].key, doc->metadata->fields[i].string_val);
                else if (doc->metadata->fields[i].type == CAX_TYPE_POINTER) cax_object_set_pointer(context, doc->metadata->fields[i].key, doc->metadata->fields[i].pointer_val);
            }
        }
        CAXArray *tags_arr = cax_object_get_pointer(context, "tags");
        if (tags_arr) { char *tags_html = build_tags_html(tags_arr); cax_object_set_string(context, "tags_html", tags_html); free(tags_html); }
        else cax_object_set_string(context, "tags_html", "");
        char slug[1024];
        strncpy(slug, entry->d_name, sizeof(slug) - 1);
        slug[sizeof(slug) - 1] = '\0';
        char *dot = strrchr(slug, '.'); if (dot) *dot = '\0';
        char url_buf[1024];
        snprintf(url_buf, sizeof(url_buf), "/%s/%s/", coll_name, slug);
        cax_object_set_string(context, "url", url_buf);
        cax_object_set_string(context, "permalink", url_buf);
        cax_object_set_string(context, "slug", slug);
        int current_index = -1;
        for (int i = 0; i < master_array->count; i++) {
            const char *u = cax_object_get_string(master_array->items[i], "url");
            if (u && strcmp(u, url_buf) == 0) { current_index = i; break; }
        }
        if (current_index > 0) {
            CAXObject *prev_item = master_array->items[current_index - 1];
            const char *p_url = cax_object_get_string(prev_item, "url");
            const char *p_title = cax_object_get_string(prev_item, "title");
            char prev_buf[512];
            snprintf(prev_buf, sizeof(prev_buf), "<a href=\"%s\">&larr; %s</a>", p_url? p_url : "", p_title? p_title : "");
            cax_object_set_string(context, "prev_link", prev_buf);
        }
        if (current_index >= 0 && current_index < master_array->count - 1) {
            CAXObject *next_item = master_array->items[current_index + 1];
            const char *n_url = cax_object_get_string(next_item, "url");
            const char *n_title = cax_object_get_string(next_item, "title");
            char next_buf[512];
            snprintf(next_buf, sizeof(next_buf), "<a href=\"%s\">%s &rarr;</a>", n_url? n_url : "", n_title? n_title : "");
            cax_object_set_string(context, "next_link", next_buf);
        }
        char *html_body = cax_markdown_to_html(doc->body);
        if (html_body) { cax_object_set_string(context, "content", html_body); free(html_body); }
        const char *raw_layout = cax_object_get_string(context, "layout");
        char layout_buf[256] = {0};
        if (raw_layout) { strncpy(layout_buf, raw_layout, sizeof(layout_buf) - 1); clean_trim(layout_buf); }
        if (strlen(layout_buf) == 0) strcpy(layout_buf, "default.cax");
        char layout_path[2048];
        snprintf(layout_path, sizeof(layout_path), "templates/layouts/%s", layout_buf);
        char *final_html = cax_template_render_file(layout_path, context);
        if (!final_html) final_html = cax_template_render_file("templates/layouts/default.cax", context);
        if (!final_html) final_html = cax_strdup("<h1>{{ title }}</h1>{{ content }}");
        char out_dir[1024];
        snprintf(out_dir, sizeof(out_dir), "site/%s/%s", coll_name, slug);
        cax_create_directory(out_dir);
        char out_path[2048];
        snprintf(out_path, sizeof(out_path), "%s/index.html", out_dir);
        cax_write_file(out_path, final_html);
        free(final_html);
        cax_object_free(context);
        cax_frontmatter_free(doc);
    }
    closedir(dir);
}
void cax_builder_run(void) {
    cax_create_directory("site");
    minify_and_copy_assets();
    CAXObject *global_data = cax_object_new();
    cax_load_all_data("_data", global_data);
    CAXObject *pagination_controllers = cax_object_new();
    CAXObject *collections_obj = cax_object_new();
    cax_object_set_pointer(global_data, "collections", collections_obj);
    DIR *scan = opendir("content");
    if(scan){
        struct dirent *e;
        while((e=readdir(scan))!=NULL){
            if(e->d_name[0]=='.') continue;
            char fp[2048]; snprintf(fp,sizeof(fp),"content/%s",e->d_name);
            if(cax_is_directory(fp)) continue;
            char *ext = strrchr(e->d_name,'.');
            if(!ext || strcmp(ext,".md")!=0) continue;
            char *fc = cax_read_file(fp);
            if(!fc) continue;
            CAXParsedDocument *doc = cax_frontmatter_parse(fc);
            free(fc);
            if(!doc ||!doc->metadata){ if(doc) cax_frontmatter_free(doc); continue; }
            const char *coll = cax_object_get_string(doc->metadata,"collection");
             if(coll){
                CAXObject *ctrl = cax_object_new();
                for(int i=0;i<doc->metadata->count;i++){
                    CAXField *f = &doc->metadata->fields[i];
                    if(strcmp(f->key,"collection")==0) continue;
                    if(f->type==CAX_TYPE_STRING) cax_object_set_string(ctrl,f->key,f->string_val);
                    else cax_object_set_pointer(ctrl,f->key,f->pointer_val);
                }
                if(doc->body) cax_object_set_string(ctrl,"content",doc->body);
                cax_object_set_pointer(pagination_controllers, coll, ctrl);
            }
            cax_frontmatter_free(doc);
        }
        closedir(scan);
    }
    CAXArray *all_collections_master = cax_array_new();
    DIR *content_dir = opendir("content");
    if (content_dir) {
        struct dirent *entry;
        while ((entry = readdir(content_dir))!= NULL) {
            if (entry->d_name[0] == '.') continue;
            char sub_path[8192];
            snprintf(sub_path, sizeof(sub_path), "content/%s", entry->d_name);
            if (cax_is_directory(sub_path)) {
                CAXArray *coll_array = cax_array_new();
                process_collection_dir(sub_path, entry->d_name, coll_array);
                cax_object_set_pointer(collections_obj, entry->d_name, coll_array);
                cax_object_set_pointer(global_data, entry->d_name, coll_array);
            }
        }
        rewinddir(content_dir);
        while ((entry = readdir(content_dir))!= NULL) {
            if (entry->d_name[0] == '.') continue;
            char sub_path[8192];
            snprintf(sub_path, sizeof(sub_path), "content/%s", entry->d_name);
            if (cax_is_directory(sub_path)) {
                CAXArray *coll_array = (CAXArray*)cax_object_get_pointer(collections_obj, entry->d_name);
                if (!coll_array) continue;
                render_collection_items(sub_path, entry->d_name, coll_array, global_data);
                for (int i = 0; i < coll_array->count; i++) cax_array_add(all_collections_master, coll_array->items[i]);
                char out_sub_dir[512];
                snprintf(out_sub_dir, sizeof(out_sub_dir), "site/%s", entry->d_name);
                cax_create_directory(out_sub_dir);
                int per_page = 5;
                char list_tpl_name[256] = "posts-list.cax";
                CAXObject *ctrl = cax_object_get_pointer(pagination_controllers, entry->d_name);
                if(ctrl){
                    const char *pp = cax_object_get_string(ctrl,"pagination");
                    if(pp) per_page = atoi(pp);
                    const char *ll = cax_object_get_string(ctrl,"layout");
                    if(ll) strncpy(list_tpl_name, ll, sizeof(list_tpl_name)-1);
                } else {
                    char tmp[512];
                    get_coll_list_template(entry->d_name, tmp, sizeof(tmp));
                    strncpy(list_tpl_name, tmp, sizeof(list_tpl_name)-1);
                }
                char full_list_tpl[1024];
                snprintf(full_list_tpl, sizeof(full_list_tpl), "templates/layouts/%s", list_tpl_name);
                if(!file_exists(full_list_tpl)){
                    snprintf(full_list_tpl, sizeof(full_list_tpl), "%s", list_tpl_name);
                    if(!file_exists(full_list_tpl)) strcpy(full_list_tpl, "templates/layouts/default.cax");
                }
                cax_pagination_process(coll_array, per_page, out_sub_dir, full_list_tpl, global_data, pagination_controllers, entry->d_name);
                printf(" + %s (%d)\n", entry->d_name, coll_array->count);
            } else {
                char *ext = strrchr(entry->d_name, '.');
                if (ext && strcmp(ext, ".md") == 0) {
                    char file_path[8192];
                    snprintf(file_path, sizeof(file_path), "content/%s", entry->d_name);
                    char *file_content = cax_read_file(file_path);
                    if (file_content) {
                        CAXParsedDocument *doc = cax_frontmatter_parse(file_content);
                        free(file_content);
                        if (doc) {
                            const char *is_coll = NULL;
                            if(doc->metadata) is_coll = cax_object_get_string(doc->metadata,"collection");
                            if(is_coll){ cax_frontmatter_free(doc); continue; }
                            CAXObject *ctx = cax_object_new();
                            copy_all_data_to_context(global_data, ctx);
                            flatten_site_from_metadata(global_data, ctx);
                            if (doc->metadata) {
                                for (int i = 0; i < doc->metadata->count; i++) {
                                    if (doc->metadata->fields[i].type == CAX_TYPE_STRING) cax_object_set_string(ctx, doc->metadata->fields[i].key, doc->metadata->fields[i].string_val);
                                    else if (doc->metadata->fields[i].type == CAX_TYPE_POINTER) cax_object_set_pointer(ctx, doc->metadata->fields[i].key, doc->metadata->fields[i].pointer_val);
                                }
                            }
                            CAXArray *root_tags_arr = cax_object_get_pointer(ctx, "tags");
                            if (root_tags_arr) { char *tags_html = build_tags_html(root_tags_arr); cax_object_set_string(ctx, "tags_html", tags_html); free(tags_html); }
                            else cax_object_set_string(ctx, "tags_html", "");
                            char rel_name[1024];
                            strncpy(rel_name, entry->d_name, sizeof(rel_name) - 1);
                            rel_name[sizeof(rel_name) - 1] = '\0';
                            char *dot = strrchr(rel_name, '.'); if (dot) *dot = '\0';
                            char url_buf[2048];
                            if (strcmp(rel_name, "index") == 0) snprintf(url_buf, sizeof(url_buf), "/");
                            else snprintf(url_buf, sizeof(url_buf), "/%s/", rel_name);
                            cax_object_set_string(ctx, "url", url_buf);
                            cax_object_set_string(ctx, "permalink", url_buf);
                            CAXObject *root_page_item = copy_metadata_object(ctx);
                            cax_array_add(all_collections_master, root_page_item);
                            char *html_body = cax_markdown_to_html(doc->body);
                            if (html_body) { cax_object_set_string(ctx, "content", html_body); free(html_body); }
                            const char *raw_layout = cax_object_get_string(ctx, "layout");
                            char layout_buf[256] = {0};
                            if (raw_layout) { strncpy(layout_buf, raw_layout, sizeof(layout_buf) - 1); clean_trim(layout_buf); }
                            if (strlen(layout_buf) == 0) strcpy(layout_buf, "default.cax");
                            char layout_path[2048];
                            snprintf(layout_path, sizeof(layout_path), "templates/layouts/%s", layout_buf);
                            char *final = cax_template_render_file(layout_path, ctx);
                            if(!final) final = cax_template_render_file("templates/layouts/default.cax", ctx);
                            if(!final) final = cax_strdup("<h1>{{ title }}</h1>{{ content }}");
                            if (strcmp(rel_name, "index") == 0) cax_write_file("site/index.html", final);
                            else {
                                char page_dir[2048]; snprintf(page_dir, sizeof(page_dir), "site/%s", rel_name);
                                cax_create_directory(page_dir);
                                char page_file[4096]; snprintf(page_file, sizeof(page_file), "%s/index.html", page_dir);
                                cax_write_file(page_file, final);
                            }
                            free(final);
                            cax_object_free(ctx);
                            cax_frontmatter_free(doc);
                        }
                    }
                }
            }
        }
        closedir(content_dir);
    }
    cax_tags_generate(all_collections_master, "site", global_data);
    cax_sitemap_generate_full(all_collections_master, global_data, "site", pagination_controllers);
    for (int i = 0; i < all_collections_master->count; i++) if (all_collections_master->items[i]) cax_object_free(all_collections_master->items[i]);
    cax_array_free(all_collections_master);
    cax_object_free(global_data);
    cax_object_free(pagination_controllers);
    remove_livereload_script();

#ifdef _WIN32
    printf("\n------------------------------------------\n");
    printf(" CAX v0.4s C Static Site Generator\n");
    printf(" by Axcora Technology - cax.axcora.com\n");
    printf("------------------------------------------\n");
    printf(" [SUCCESS] Build completed in site/\n");
    printf(" -> pretty urls enabled\n\n");
#else
    printf("\n\x1b[90m------------------------------------------\x1b[0m\n");
    printf(" \x1b[1;36mCAX\x1b[0m \x1b[90mv0.4s\x1b[0m C Static Site Generator\n");
    printf(" \x1b[2mby Axcora Technology - cax.axcora.com\x1b[0m\n");
    printf("\x1b[90m------------------------------------------\x1b[0m\n");
    printf(" \x1b[32m✔ Build completed\x1b[0m in site/\n\n");
#endif
}

void cax_builder_start_server(void) {
    cax_builder_run(); // BERSIH - GAK ADA LIVE RELOAD
#ifdef _WIN32
    printf("\n [SERVER] http://localhost:8080\n");
    printf(" [INFO] Serving site/ folder\n\n");
    system("start http://localhost:8080");
    system("php -S localhost:8080 -t site");
#else
    printf("\n \x1b[1m CAX Server running at:\x1b[0m\n");
    printf(" \x1b[36m> Local: \x1b[1mhttp://localhost:8080/\x1b[0m\n\n");
    system("python3 -m http.server 8080 --directory site/");
#endif
}
void cax_builder_serve(void){ cax_builder_start_server(); }

void cax_builder_start_with_watch(void){
    cax_builder_run();
    inject_livereload_script();
#ifdef _WIN32
    printf("\n [WATCH] Live Reload Mode\n");
    printf(" [SERVE] http://localhost:8080\n");
    printf(" [WATCH] content/ templates/ _data/ public/\n\n");
    system("start http://localhost:8080");
    STARTUPINFO si={0}; PROCESS_INFORMATION pi={0}; si.cb=sizeof(si);
    char cmd[512]; snprintf(cmd,sizeof(cmd),"cmd /c php -S localhost:8080 -t site");
    if(!CreateProcess(NULL,cmd,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi)){
        snprintf(cmd,sizeof(cmd),"cmd /c python -m http.server 8080 --directory site");
        CreateProcess(NULL,cmd,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi);
    }
    time_t lc=scan_dir("content"); time_t lt=scan_dir("templates");
    time_t ld=scan_dir("_data"); time_t lp=scan_dir("public");
    while(1){
        Sleep(1000);
        time_t c=scan_dir("content"); time_t t=scan_dir("templates");
        time_t d=scan_dir("_data"); time_t p=scan_dir("public");
        if(c!=lc||t!=lt||d!=ld||p!=lp){
            printf(" [CHANGE] rebuilding...\n");
            cax_builder_run();
            inject_livereload_script();
            printf(" [DONE] rebuilt\n");
            lc=c; lt=t; ld=d; lp=p;
        }
    }
#else
    printf("\n \x1b[1;36mCAX\x1b[0m Watching...\n");
    printf(" \x1b[2mcontent/ templates/ _data/ public/\x1b[0m\n");
    printf(" \x1b[36m> http://localhost:8080/\x1b[0m\n\n");
    system("xdg-open http://localhost:8080 2>/dev/null &");
    if(fork()==0){ execlp("python3","python3","-m","http.server","8080","--directory","site",NULL); exit(0); }
    time_t lc=scan_dir("content"); time_t lt=scan_dir("templates"); time_t ld=scan_dir("_data"); time_t lp=scan_dir("public");
    while(1){
        sleep(1);
        time_t c=scan_dir("content"); time_t t=scan_dir("templates"); time_t d=scan_dir("_data"); time_t p=scan_dir("public");
        if(c!=lc||t!=lt||d!=ld||p!=lp){
            printf(" [change] rebuilding...\n");
            cax_builder_run();
            inject_livereload_script(); // <--- REBUILD + INJECT LAGI!
            printf(" ✔ rebuilt\n"); lc=c; lt=t; ld=d; lp=p;
        }
    }
#endif
}