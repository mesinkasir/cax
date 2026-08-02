#include "sitemap.h"
#include "../utils/file_utils.h"
#include "../data/data_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cax_sitemap_generate(CAXObject *global_data, const char *output_dir){}

void cax_sitemap_generate_full(CAXArray *all_pages, CAXObject *global_data, const char *output_dir, CAXObject *controllers){
    CAXObject *metadata = cax_object_get_pointer(global_data,"metadata");
    CAXObject *site = metadata? cax_object_get_pointer(metadata,"site"):NULL;
    const char *base_url = site? cax_object_get_string(site,"url"):"http://localhost:8080";
    if(!base_url) base_url="http://localhost:8080";
    const char *title = site? cax_object_get_string(site,"title"):"CAX SSG";
    const char *desc = site? cax_object_get_string(site,"description"):"CAX Static Site";
    if(!title) title="CAX SSG";
    if(!desc) desc="High performance SSG in C";

    // ========== SITEMAP.XML ==========
    char *xml = malloc(1024*1024*4);
    strcpy(xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");
    for(int i=0;i<all_pages->count;i++){
        CAXObject *p = all_pages->items[i];
        const char *url = cax_object_get_string(p,"url");
        if(!url) continue;
        char line[2048];
        snprintf(line,sizeof(line)," <url><loc>%s%s</loc></url>\n",base_url,url);
        strcat(xml,line);
    }
    if(controllers){
        for(int i=0;i<controllers->count;i++){
            const char *coll = controllers->fields[i].key;
            CAXObject *ctrl = controllers->fields[i].pointer_val;
            const char *pag_s = cax_object_get_string(ctrl,"pagination");
            int per = pag_s? atoi(pag_s):5;
            int total=0;
            for(int j=0;j<all_pages->count;j++){
                CAXObject *p = all_pages->items[j];
                const char *u = cax_object_get_string(p,"url");
                if(u && strstr(u, coll)) total++;
            }
            int pages = (total+per-1)/per;
            for(int pg=2; pg<=pages; pg++){
                char line[512];
                snprintf(line,sizeof(line)," <url><loc>%s/%s/page/%d/</loc></url>\n",base_url,coll,pg);
                strcat(xml,line);
            }
        }
    }
    strcat(xml,"</urlset>\n");
    char path[1024]; snprintf(path,sizeof(path),"%s/sitemap.xml",output_dir);
    cax_write_file(path, xml);
    free(xml);

    // ========== FEED.XML & RSS.XML ==========
    char *feed = malloc(1024*1024*4);
    strcpy(feed, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<rss version=\"2.0\"><channel>\n");
    char tmp[2048];
    snprintf(tmp,sizeof(tmp),"<title>%s</title><link>%s</link><description>%s</description>\n",title,base_url,desc);
    strcat(feed,tmp);
    for(int i=0;i<all_pages->count && i<20;i++){
        CAXObject *p = all_pages->items[i];
        const char *url = cax_object_get_string(p,"url");
        const char *t = cax_object_get_string(p,"title");
        const char *d = cax_object_get_string(p,"description");
        if(!url||!t) continue;
        if(strcmp(url,"/")==0) continue;
        char item[4096];
        snprintf(item,sizeof(item),"<item><title><![CDATA[%s]]></title><link>%s%s</link><description><![CDATA[%s]]></description></item>\n",t,base_url,url,d?d:"");
        strcat(feed,item);
    }
    strcat(feed,"</channel></rss>\n");
    char fpath[1024];
    snprintf(fpath,sizeof(fpath),"%s/feed.xml",output_dir); cax_write_file(fpath, feed);
    snprintf(fpath,sizeof(fpath),"%s/rss.xml",output_dir); cax_write_file(fpath, feed);
    free(feed);

    // ========== FEED.JSON (Eleventy style) ==========
    char *json = malloc(1024*1024*4);
    strcpy(json, "{\n");
    snprintf(tmp,sizeof(tmp)," \"version\": \"https://jsonfeed.org/version/1.1\",\n \"title\": \"%s\",\n \"home_page_url\": \"%s\",\n \"feed_url\": \"%s/feed.json\",\n \"description\": \"%s\",\n \"items\": [\n",title,base_url,base_url,desc);
    strcat(json,tmp);
    int first=1;
    for(int i=0;i<all_pages->count && i<20;i++){
        CAXObject *p = all_pages->items[i];
        const char *url = cax_object_get_string(p,"url");
        const char *t = cax_object_get_string(p,"title");
        const char *d = cax_object_get_string(p,"description");
        if(!url||!t) continue;
        if(strcmp(url,"/")==0) continue;
        if(!first) strcat(json,",\n");
        char item[4096];
        snprintf(item,sizeof(item)," {\"id\":\"%s%s\",\"url\":\"%s%s\",\"title\":\"%s\",\"content_text\":\"%s\"}",base_url,url,base_url,url,t,d?d:"");
        strcat(json,item);
        first=0;
    }
    strcat(json,"\n ]\n}\n");
    snprintf(fpath,sizeof(fpath),"%s/feed.json",output_dir); cax_write_file(fpath, json);
    free(json);

    // ========== ROBOTS.TXT ==========
    char robots[2048];
    snprintf(robots,sizeof(robots),"User-agent: *\nAllow: /\n\nSitemap: %s/sitemap.xml\n",base_url);
    snprintf(path,sizeof(path),"%s/robots.txt",output_dir);
    cax_write_file(path, robots);

    printf(" ✓ sitemap.xml + feed.xml + rss.xml + feed.json + robots.txt\n");
}