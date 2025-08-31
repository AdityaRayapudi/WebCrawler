from fastapi import FastAPI
from pydantic import BaseModel
import asyncio

# from dynamic_scraper import BrandPage
from scraper_manager import ScraperManager, WebPage

class URL(BaseModel):
    seed: str
    link: str

# brand = BrandPage()
scraper_manager = ScraperManager()

app = FastAPI()

@app.post("/parsed-pages/")
async def parse_page(url: URL):
    url_dict = url.model_dump()
    seed, link = url_dict.get("seed"), url_dict.get("link")

    await scraper_manager.start_browser(proxy_server = "https://127.0.0.1:8080", is_headless=True)


    page : WebPage = await scraper_manager.open_page(seed, link)

    await page.extract_data(seed=seed)

    page_details = await page.get_final_details()

    await scraper_manager.close()

    return page_details
    # return page_details

# @app.post("/parsed-pages/")
# async def parse_page(url: URL):
#     url_dict = url.model_dump()
#     ip, seed, q = [url_dict.get("ip"), url_dict.get("seed"), url_dict.get("q")]
#     page_data = await brand.open_page(ip = ip, seed= seed, link=q)

#     return page_data