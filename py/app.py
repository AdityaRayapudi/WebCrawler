from fastapi import FastAPI
from contextlib import asynccontextmanager
from pydantic import BaseModel
import asyncio
from playwright.async_api import async_playwright, Playwright
from playwright_stealth import Stealth

from dynamic_scraper import BrandPage


class URL(BaseModel):
    ip: str
    seed: str
    q: str

brand = BrandPage()

# @asynccontextmanager
# async def lifespan(app: FastAPI):
#     with Stealth().use_async(async_playwright()) as playwright:
            

app = FastAPI()

@app.post("/parsed-pages/")
async def parse_page(url: URL):
    url_dict = url.model_dump()
    ip, seed, q = [url_dict.get("ip"), url_dict.get("seed"), url_dict.get("q")]
    page_data = await brand.open_page(ip = ip, seed= seed, link=q)

    return page_data