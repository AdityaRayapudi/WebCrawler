from fastapi import FastAPI
from pydantic import BaseModel

from scraper_manager import ScraperManager, WebPage

class URL(BaseModel):
    seed: str
    link: str

scraper_manager = ScraperManager()

app = FastAPI()

@app.get("/")
async def test():
    return ({"message" : "Connected"})

@app.post("/parsed-pages/", status_code=201)
async def parse_page(url: URL):
    url_dict = url.model_dump()
    seed, link = url_dict.get("seed"), url_dict.get("link")

    await scraper_manager.start_browser(proxy_server = "https://proxy:8080", is_headless=True)

    page : WebPage = await scraper_manager.open_page(seed, link)

    await page.extract_data(seed=seed)

    page_details = await page.get_final_details()

    await scraper_manager.close()

    return page_details