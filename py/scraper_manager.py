from playwright.async_api import async_playwright
from playwright_stealth import Stealth
from lxml import html

import time
import json
import hashlib

import asyncio

class ScraperManager:
    async def start_browser(self, proxy_server, is_headless = True):
        playwright = await async_playwright().start()
        
        self.browser = await playwright.chromium.launch(
            proxy= {
                "server" : proxy_server
            },
            args = [
                "--ignore-certificate-errors",
                "--disable-blink-features=AutomationControlled",
                "--disable-http2"
            ],
            headless = is_headless,
        )
            
    async def open_page(self, seed, link):
        page = await self.browser.new_page()
        await Stealth().apply_stealth_async(page)
        # await Stealth().use_async(page)

        await page.goto(f"https://{seed}{link}")
        await page.evaluate("document.body.style.zoom='0.5'");

        print(f"{seed} ready to scrape...")
        return WebPage(page=page, seed=seed)

    async def close(self):
        await self.browser.close()


class WebPage:
    def __init__(self, page, seed):
        self.page = page
        self.html = ""

        self.ld_json = None
        self.details = {
                "md5" : "",
                "is_product" : False,

                "product" : {
                    "name" : None,
                    "price" : None,
                    "description" : "",
                    "composition" : None,
                    "image" : None,
                    "scrapped_images" : [],
                },

                "urls" : []
        }

    async def get_page(self):
        return self.page
    
    async def get_final_details(self):
        await self.close()
        return self.details

    async def extract_data(self, seed):
        self.html = html.fromstring(await self.page.content())

        self.ld_json = await WebPage.get_product_json(self.html)
        

        # If ld json exits then the current page is a product
        is_product_page = self.ld_json is not None

        if is_product_page:
            # Get all product info and add to details
            await self.extract_product_data()

            # Hash collected data with md5
            md5 = hashlib.md5(str(self.details["product"]).encode())
            self.details["md5"] = md5.hexdigest()
        else:
            # Scroll page to explore all data
            await self.scroll_content()
        
        urls = await self.get_relative_urls(self.html, seed)
        self.details["urls"].append(urls)


    async def extract_product_data(self):
        print("locating product data...")
        self.details["is_product"] = True

        # Extract info from self.ld_json
        self.details["product"]["name"] = self.ld_json["name"]                  
        self.details["product"]["description"] = self.ld_json["description"]
        self.details["product"]["image"] = self.ld_json["image"]

        if "price" in self.ld_json:
            self.details["product"]["price"] = self.ld_json["price"]  

        # Extract info from metadata
        metadata = await WebPage.get_metadata(self.html)
        if len(metadata["description"]) > len(self.details["product"]["description"]):
            self.details["product"]["description"] = metadata["description"]

        if not self.details["product"]["price"] and metadata["price"]:
            self.details["product"]["price"] = metadata["price"]
        else:
            self.details["product"]["price"] = self.ld_json["offers"]["lowPrice"]

        print("metadata extracted...")

        # Set compostion
        composition = await WebPage.get_composition(self.page)
        self.details["product"]["composition"] = composition

        print("composition extracted...")

        # Set scrapped images
        images = await WebPage.get_images(self.page, self.ld_json["name"])
        self.details["product"]["scrapped_images"].append(images)

        print("images extracted...")


    async def scroll_content(self, scroll_time = 3):
        print("scrolling content...")

        start_time = time.time()

        unexplored_content = True
        previous_time = start_time
        previous_height = await self.page.evaluate("document.body.scrollHeight")

        while unexplored_content:
            await self.page.mouse.wheel(0, 1000)
            current_height = await self.page.evaluate("document.body.scrollHeight")
            current_time = time.time()

            print("More Content :", not (current_height == previous_height) )
            print("Time passed :", current_time - previous_time)

            # If 3 seconds have elapsed and scroll heights has not changed end loop
            if ((current_height == previous_height) and (current_time - previous_time >= scroll_time)):
                unexplored_content = False
            # Set new previous time & height if no new content is loaded
            elif (current_height != previous_height):
                if self.page.is_visible("text='Load More'"):
                    self.page.hover("text='Load More'")
                    self.page.click("text='Load More'")
                previous_time = current_time
                previous_height = current_height

        # Update page html with new content
        self.html = html.fromstring(await self.page.content())

    @staticmethod
    async def get_product_json(html):
        context = html.xpath('//script[@type="application/ld+json"]')
        # If no json-ld description is found, current page is not product page
        if len(context) == 0:
            return None
        
        for desc in context:
            desc_json = json.loads(desc.text_content())
            if "@type" in desc_json and desc_json["@type"] == "Product":
                return desc_json
            
    @staticmethod
    async def get_relative_urls(html, seed):
        all_urls = html.xpath("//a/@href")
        relative_urls = []

        for url in all_urls:
            if url.startswith("/"):
                relative_urls.append(url)
                continue

            if seed in url:
                url = url.removeprefix("https://" + seed)
                relative_urls.append(url)

        return relative_urls

    @staticmethod
    async def get_metadata(html):
        metadata = html.xpath('//meta')
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
    
    @staticmethod
    async def get_images(page, product_name):
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
        
    @staticmethod
    async def get_composition(page):
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
    
    async def close(self):
        await self.page.close()

async def main():
    scraper_manager = ScraperManager()
    await scraper_manager.start_browser(proxy_server = "https://127.0.0.1:8080", is_headless=False)
    
    page : WebPage = await scraper_manager.open_page("www.nike.com", "/t/vomero-plus-mens-road-running-shoes-5npsVBwT/HV8150-100")

    await scraper_manager.close()
    

if __name__ == "__main__":
    asyncio.run(main())