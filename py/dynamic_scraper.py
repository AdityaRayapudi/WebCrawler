from playwright.async_api import async_playwright, Playwright
from playwright_stealth import Stealth
from lxml import html

import time
import json


class BrandPage:
    async def open_page(self, ip : str, seed : str, link: str):
        async with Stealth().use_async(async_playwright()) as playwright:

            product_json = None

            page_details = {
                "is_product" : False,

                "product" : {
                    "name" : None,
                    "price" : None,
                    "description" : None,
                    "composition" : None,
                    "image" : None,
                    "scrapped_images" : [],
                },

                "urls" : []

            }

            self.browser = await playwright.chromium.launch(
                args = [
            f"--host-resolver-rules=MAP {seed} {ip}",
            "--ignore-certificate-errors",
            "--disable-blink-features=AutomationControlled",
                ],
                # headless=False
            )

            page = await self.browser.new_page()

            await page.goto(f"https://{seed}{link}")
            # await page.wait_for_load_state('networkidle')

            
            await page.evaluate("document.body.style.zoom='0.5'");

            print(f"{seed} ready to scrape...")

            page_html = html.fromstring(await page.content())
            product_json = await self.get_product_json(page_html)

            if product_json == None:
                print("scrolling content...")
                await self.scroll_content(page)
            else:
                print("locating product data...")
                page_details["is_product"] = True

                # Extract info from product_json
                page_details["product"]["name"] = product_json["name"]                  
                page_details["product"]["description"] = product_json["description"]
                page_details["product"]["image"] = product_json["image"]

                if "price" in product_json:
                    page_details["product"]["price"] = product_json["price"]  

                 # Extract info from metadata
                metadata = await self.get_metadata(page_html)
                if len(metadata["description"]) > len(page_details["product"]["description"]):
                    page_details["product"]["description"] = metadata["description"]

                if not page_details["product"]["price"] and metadata["price"]:
                    page_details["product"]["price"] = metadata["price"]
                else:
                    page_details["product"]["price"] = product_json["offers"]["lowPrice"]

                print("metadata extracted...")

                # Set compostion
                composition = await self.get_composition(page)
                page_details["product"]["composition"] = composition

                print("composition extracted...")

                # Set scrapped images
                images = await self.get_images(page, product_json["name"])
                page_details["product"]["scrapped_images"].append(images)

                print("images extracted...")


            page_html = html.fromstring(await page.content())

            urls = await self.get_relative_urls(page_html, seed)
            page_details["urls"].append(urls)

            await self.close_page()
            return page_details

    async def get_product_json(self, page_html):
        context = page_html.xpath('//script[@type="application/ld+json"]')
        # If no json-ld description is found, current page is not product page
        if len(context) == 0:
            return None
        
        for desc in context:
            desc_json = json.loads(desc.text_content())
            if "@type" in desc_json and desc_json["@type"] == "Product":
                return desc_json
            
    async def scroll_content(self, page):
        start_time = time.time()

        unexplored_content = True
        previous_time = start_time
        previous_height = await page.evaluate("document.body.scrollHeight")

        while unexplored_content:
            await page.mouse.wheel(0, 1000)
            current_height = await page.evaluate("document.body.scrollHeight")
            current_time = time.time()

            print("More Content :", not (current_height == previous_height) )
            print("Time passed :", current_time - previous_time)

            # If 3 seconds have elapsed and scroll heights has not changed end loop
            if ((current_height == previous_height) and (current_time - previous_time >= 3)):
                unexplored_content = False
            # Set new previous time & height if no new content is loaded
            elif (current_height != previous_height):
                if page.is_visible("text='Load More'"):
                    page.hover("text='Load More'")
                    page.click("text='Load More'")
                previous_time = current_time
                previous_height = current_height

    async def get_relative_urls(self, page_html, seed):
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

    async def get_metadata(self, page_html):
        metadata = page_html.xpath('//meta')
        properties = {
            "title",
            "description",
            "price",
        }

        out = {
            "title" : "",
            "description" : "",
            "price" : None
        }

        for i in metadata:
            prop = i.get("property")
            if prop in properties:
                out[prop] = i.get("content")
            else:
                out[prop] = None

        return out
    
    async def get_images(self, page, product_name):
        imgs = page.locator("xpath=//img")
        product_imgs = []

        for i in range(await imgs.count()):
            img = imgs.nth(i)

            alt = await img.get_attribute("alt") or ""
            aria_label = await img.get_attribute("aria-label") or ""
            current_src = ""

            # Skip unlabled imgs
            if not alt and not aria_label:
                continue
            
            # Check if label for image contains the product name
            if product_name in alt or product_name in aria_label:
                # Get only the current img url to counter lazy-loading
                current_src = await img.evaluate('img => img.currentSrc')
            
            # If src is not empty append to output
            if current_src:
                width = await img.evaluate('img => img.naturalWidth')
                height = await img.evaluate('img => img.naturalWidth')

                product_imgs.append([width, height, current_src])

        return product_imgs
        

    async def get_composition(self, page):
        compostion_buttons = [
            "composition",
            "fabric",
            "details",
        ]

        fabrics = [ "cotton", "elastane", "wool", "polyester", "silk", "linen", "nylon", "denim", "chiffon", "cashmere", "satin", "spandex", "tweed"]

        compostion = None

        # Case insenstive with Regex for product details or composition button
        for button in compostion_buttons:
            if await page.is_visible(f"text='/{button}/i'"):
                await page.hover(f"text='/{button}/i'")
                await page.click(f"text='/{button}/i'")

        for fab in fabrics:
            compostion = page.get_by_text(fab)

            if compostion == None:
                continue

            for i in await compostion.all_inner_texts():
                if "%" in i:
                    return i.strip()
        

    async def close_page(self):
         await self.browser.close()
         print("...closed page")


# async def main():
#     async with Stealth().use_async(async_playwright()) as playwright:
#         brand = BrandPage()
#         # await brand.open_page(playwright, "23.34.164.187", "www.nike.com", "/w/mens-socks-7ny3qznik1")
#         dockers_data = await brand.open_page(playwright, "23.227.38.74", "us.dockers.com", "/products/ultimate-chino-slim-fit-794880220?variant=44611662905505&utm_source=google&utm_medium=cpc&adpos=&scid=scplp79488022003330&sc_intid=79488022003330&utm_source=google&utm_medium=cpc&utm_campaign=pmax-brand-mens-pants&gad_source=1&gad_campaignid=18154247935&gbraid=0AAAAAC-PRd6p8lSguPbrFVZzOVNr45XAM&gclid=Cj0KCQjwh5vFBhCyARIsAHBx2wwjQOEoYdpI6ejAhlEMOLesKQxj8O2f00UtUeAPFCQrGouB3KzYqkEaAmsNEALw_wcB")
#         print(dockers_data)
        # await brand.open_page(playwright, "23.221.225.98", "www.calvinklein.us", "/en/men/apparel/tops/liquid-touch-slim-t-shirt-/4LC250G-LDY.html")
        # await brand.open_page(playwright, "23.34.164.187", "www.nike.com", "/t/everyday-plus-cushioned-training-crew-socks-6-pairs-vlRw4Q/SX6897-100")
        # await brand.open_page(playwright, "23.34.165.183", "bananarepublic.gap.com", "/browse/product.do?pid=795943032&vid=1&pcid=10894&cid=10894&nav=meganav%3AMen%3AMEN%27S+CLOTHING%3APolos#pdp-page-content")


# if __name__ == "__main__":
    
#         print("Hello World")