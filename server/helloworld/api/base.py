from flask import Flask, Blueprint

from helloworld.api.routes import database
from helloworld.api.routes import device


blueprint: Blueprint = Blueprint('api', __name__, url_prefix='/api')

blueprint.register_blueprint(database.blueprint)
blueprint.register_blueprint(device.blueprint)


def setup_api_routes(application: Flask) -> None:
    application.register_blueprint(blueprint)