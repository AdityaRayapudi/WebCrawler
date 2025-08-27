import time
import json
from playwright.sync_api import sync_playwright
from playwright_stealth import Stealth
from lxml import html

def parse_page(ip : str, seed : str, link: str):
    with Stealth().use_sync(sync_playwright()) as p:
        browser = p.chromium.launch(
            args = [
        f"--host-resolver-rules=MAP {seed} {ip}",
        "--ignore-certificate-errors",
        "--disable-blink-features=AutomationControlled"
            ],
            headless=False
        )

        page = browser.new_page()

        start_time = time.time()

        page.goto(f"https://{seed}{link}")
        page.evaluate("document.body.style.zoom='0.5'");

        
        unexplored_content = True
        previous_time = start_time
        previous_height = page.evaluate("document.body.scrollHeight")

        while unexplored_content:
            page.mouse.wheel(0, 1000)
            current_height = page.evaluate("document.body.scrollHeight")
            current_time = time.time()

            print("More Content :", not (current_height == previous_height) )
            print("Time passed :", current_time - previous_time)

            # If 3 seconds have elapsed and scroll heights has not changed end loop
            if ((current_height == previous_height) and (current_time - previous_time >= 5)):
                unexplored_content = False
            # Set new previous time & height if no new content is loaded
            elif (current_height != previous_height):
                if page.is_visible("text='Load More'"):
                    page.hover("text='Load More'")
                    page.click("text='Load More'")
                previous_time = current_time
                previous_height = current_height

        html_string = page.content()
        browser.close()
        return html_string

def get_relative_urls(page_html, seed):
    all_urls = page_html.xpath("//a/@href")
    relative_urls = []

    for url in all_urls:
        if url.startswith("/"):
            relative_urls.append(url)
            continue

        if seed in url:
            url = url.removeprefix("https://" + seed)
            relative_urls.append(url)

    return relative_urls

def get_product_details(page_html):
    context = page_html.xpath('//script[@type="application/ld+json"]')
    # If no json-ld description is found, current page is not product page
    if len(context) == 0:
        return None
    
    for desc in context:
        desc_json = json.loads(desc.text_content())
        if desc_json["@type"] == "Product":
            return desc_json

def get_images(page_html, name):
    imgs = page_html.xpath("//img")
    product_imgs = []

    for img in imgs:
        alt = img.get("alt")
        aria_label = img.get("aria-label")

        if alt != None and name in alt:
            product_imgs.append(img.get("src"))
            continue

        if aria_label != None and name in aria_label:
            product_imgs.append(img.get("attr"))

        
    return product_imgs

if __name__ == "__main__":
    # page_string = parse_page("23.34.164.187", "www.nike.com", "/t/solo-swoosh-mens-cuffed-fleece-pants-pFXQdz/HV1088-222")
    page_string = parse_page("23.34.164.187", "www.nike.com", "/w/mens-socks-7ny3qznik1")

    # page_string = parse_page("23.227.38.74", "us.dockers.com", "/collections/mens-new-arrivals")
    # page_string = parse_page("23.221.225.98", "www.calvinklein.us", "/en/men/apparel/tops/liquid-touch-slim-t-shirt-/4LC250G-LDY.html")
    # page_html = html.fromstring(page_string)
    # product = get_product_details(page_html)

    # print(get_relative_urls(page_html, "www.nike.com"))
    # print(product)
    # if product["name"]:
    #     print(get_images(page_html, product["name"]))

    



    