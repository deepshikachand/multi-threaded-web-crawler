import requests
from bs4 import BeautifulSoup
import time
import random
import threading
import queue
import re
import os
from urllib.parse import urlparse, urljoin

class WebCrawler:
    def __init__(self):
        # Configuration
        self.max_threads = 4
        self.max_depth = 2
        self.allowed_domains = []
        
        # State
        self.url_queue = queue.Queue()
        self.visited_urls = set()
        self.running = False
        self.pages_crawled = 0
        self.images_saved = 0
        self.active_threads = 0
        
        # Threading
        self.threads = []
        self.queue_lock = threading.Lock()
        self.stats_lock = threading.Lock()
        
        # Create directories
        os.makedirs("data/images", exist_ok=True)
        os.makedirs("data/content", exist_ok=True)
        os.makedirs("logs", exist_ok=True)
        
        # User agent for requests
        self.user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Multi-Threaded-Web-Crawler/1.0"
        
        print("WebCrawler initialized")
    
    def set_max_threads(self, threads):
        """Set the maximum number of threads for the crawler"""
        self.max_threads = max(1, threads)
    
    def set_max_depth(self, depth):
        """Set the maximum crawl depth"""
        self.max_depth = max(1, depth)
    
    def set_allowed_domains(self, domains):
        """Set allowed domains for crawling"""
        self.allowed_domains = domains
    
    def start(self, seed_url, depth=None):
        """Start the crawler with the given seed URL"""
        if self.running:
            print("Crawler is already running")
            return
        
        # Set depth if provided
        if depth is not None:
            self.max_depth = depth
        
        # Reset state
        self.url_queue = queue.Queue()
        self.visited_urls = set()
        self.pages_crawled = 0
        self.images_saved = 0
        self.active_threads = 0
        
        # Add seed URL to queue
        self.url_queue.put((seed_url, 0))  # (url, depth)
        self.visited_urls.add(seed_url)
        
        # Start crawler
        self.running = True
        print(f"Starting crawler with {self.max_threads} threads and max depth {self.max_depth}")
        
        # Create and start worker threads
        self.threads = []
        for i in range(self.max_threads):
            thread = threading.Thread(target=self.crawl_thread)
            thread.start()
            self.threads.append(thread)
    
    def stop(self):
        """Stop the crawler"""
        if not self.running:
            return
        
        self.running = False
        print("Stopping crawler...")
        
        # Wait for all threads to finish
        for thread in self.threads:
            if thread.is_alive():
                thread.join()
        
        self.threads = []
        print("Crawler stopped")
    
    def get_queue_size(self):
        """Get the current size of the URL queue"""
        return self.url_queue.qsize()
    
    def get_pages_crawled(self):
        """Get the number of pages crawled"""
        return self.pages_crawled
    
    def get_images_saved(self):
        """Get the number of images saved"""
        return self.images_saved
    
    def get_active_threads(self):
        """Get the number of active threads"""
        return self.active_threads
    
    def get_unique_urls(self):
        """Get the number of unique URLs visited"""
        return len(self.visited_urls)
    
    def is_running(self):
        """Check if the crawler is running"""
        return self.running
    
    def crawl_thread(self):
        """Worker thread function for crawling"""
        with self.stats_lock:
            self.active_threads += 1
        
        while self.running:
            try:
                # Get URL from queue with timeout
                try:
                    url, depth = self.url_queue.get(timeout=1)
                    has_url = True
                except queue.Empty:
                    has_url = False
                
                if has_url:
                    # Process the URL
                    try:
                        self.process_url(url, depth)
                    except Exception as e:
                        print(f"Error processing {url}: {e}")
                    finally:
                        self.url_queue.task_done()
            except Exception as e:
                print(f"Error in crawl thread: {e}")
        
        with self.stats_lock:
            self.active_threads -= 1
    
    def process_url(self, url, depth):
        """Process a single URL, extracting links and images"""
        if depth > self.max_depth:
            return
        
        print(f"Processing URL: {url} (depth: {depth})")
        
        # Check if domain is allowed
        domain = urlparse(url).netloc
        domain_allowed = not self.allowed_domains  # If empty list, all domains allowed
        
        for allowed_domain in self.allowed_domains:
            if domain == allowed_domain or domain.endswith(f".{allowed_domain}"):
                domain_allowed = True
                break
        
        if not domain_allowed:
            print(f"Skipping URL from disallowed domain: {domain}")
            return
        
        # Try to fetch the page
        try:
            # Add delay to be polite
            time.sleep(random.uniform(1.0, 2.0))
            
            headers = {"User-Agent": self.user_agent}
            response = requests.get(url, headers=headers, timeout=10)
            
            if response.status_code != 200:
                print(f"Failed to fetch {url}: {response.status_code}")
                return
            
            # Parse content
            soup = BeautifulSoup(response.text, "html.parser")
            
            # Update stats
            with self.stats_lock:
                self.pages_crawled += 1
            
            # Extract title
            title = soup.title.string if soup.title else "No Title"
            print(f"Title: {title}")
            
            # Process images
            images = soup.find_all('img')
            for img in images:
                src = img.get('src')
                if src:
                    # Convert relative URL to absolute
                    image_url = urljoin(url, src)
                    alt = img.get('alt', '')
                    
                    # Simulate saving image (we won't actually download it)
                    with self.stats_lock:
                        self.images_saved += 1
                    print(f"Found image: {image_url}")
            
            # Extract links if not at max depth
            if depth < self.max_depth:
                links = soup.find_all('a')
                for link in links:
                    href = link.get('href')
                    if href:
                        # Skip anchors, javascript, etc.
                        if href.startswith('#') or href.startswith('javascript:'):
                            continue
                        
                        # Convert relative URL to absolute
                        next_url = urljoin(url, href)
                        
                        # Only add if it's a web page (not a file)
                        if self.is_webpage_url(next_url):
                            with self.stats_lock:
                                if next_url not in self.visited_urls:
                                    self.visited_urls.add(next_url)
                                    self.url_queue.put((next_url, depth + 1))
        
        except Exception as e:
            print(f"Error processing {url}: {e}")
    
    def is_webpage_url(self, url):
        """Check if a URL likely points to a webpage, not a file download"""
        parsed = urlparse(url)
        path = parsed.path.lower()
        
        # Exclude common file extensions
        excluded_extensions = ['.pdf', '.doc', '.docx', '.xls', '.xlsx', '.ppt', '.pptx', 
                              '.zip', '.rar', '.tar', '.gz', '.jpg', '.jpeg', '.png', 
                              '.gif', '.mp3', '.mp4', '.avi', '.mov']
        
        for ext in excluded_extensions:
            if path.endswith(ext):
                return False
        
        return True

def main():
    # Create and configure the crawler
    crawler = WebCrawler()
    
    # Configure crawler
    crawler.set_max_threads(4)
    crawler.set_max_depth(2)
    
    # Set allowed domains to limit scope - can be empty for no restrictions
    # We'll use MDN for demo purposes
    crawler.set_allowed_domains(["developer.mozilla.org"])
    
    # Start the crawler with a seed URL
    seed_url = "https://developer.mozilla.org/en-US/docs/Web"
    print(f"Starting crawler with seed URL: {seed_url}")
    
    # Start crawler in a separate thread
    crawler_thread = threading.Thread(target=crawler.start, args=(seed_url,))
    crawler_thread.start()
    
    # Monitor and display progress
    try:
        running = True
        while running and crawler.is_running():
            status = (f"Queue: {crawler.get_queue_size()} | "
                     f"Pages: {crawler.get_pages_crawled()} | "
                     f"Images: {crawler.get_images_saved()} | "
                     f"Threads: {crawler.get_active_threads()} | "
                     f"URLs: {crawler.get_unique_urls()}")
            
            print(f"\r{status}", end="", flush=True)
            time.sleep(1)
            
            # Stop after a certain amount of time or pages crawled
            if (crawler.get_pages_crawled() >= 20 or 
                crawler.get_queue_size() == 0 and crawler.get_active_threads() == 0):
                running = False
    
    except KeyboardInterrupt:
        print("\nUser interrupted crawling process.")
    
    # Stop crawler
    print("\nStopping crawler...")
    crawler.stop()
    
    # Display final statistics
    print("\nCrawling completed!")
    print("-------------------")
    print(f"Total pages crawled: {crawler.get_pages_crawled()}")
    print(f"Total images found: {crawler.get_images_saved()}")
    print(f"Total unique URLs: {crawler.get_unique_urls()}")

if __name__ == "__main__":
    main() 