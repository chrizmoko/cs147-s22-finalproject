from flask import Flask

from helloworld.api.base import setup_api_routes
from helloworld.views.base import setup_view_routes


app = Flask(__name__)

setup_api_routes(app)
setup_view_routes(app)