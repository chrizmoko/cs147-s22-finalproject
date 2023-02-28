from flask import Flask, Blueprint, render_template

from helloworld.api.routes.device import retrieve_metrics_internal


blueprint: Blueprint = Blueprint('userviews', __name__)


def setup_view_routes(application: Flask) -> None:
    application.register_blueprint(blueprint)


@blueprint.route('/', methods=['GET', 'POST'])
def index():
    return render_template('index.html', title='helloworld')


@blueprint.route('/heatmap/', methods=['GET', 'POST'])
def heatmap():
    return render_template('heatmap/heatmap.html', title='heatmap')


@blueprint.route('/heatmap/<mac_address>', methods=['GET', 'POST'])
def heatmap_user(mac_address):
    metrics = retrieve_metrics_internal(mac_address, 1000)

    day_headers = ('Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat')

    rows_data = []
    for hour in range(24):
        freq_columns = [metrics[day][hour] for day in day_headers]
        rows_data.append((str(hour).zfill(2), freq_columns))

    table_list = [(f'Past 2 Week Activity for {mac_address}', day_headers, rows_data)]

    return render_template('heatmap/heatmap.html',
        title='helloworld',
        mac_address=mac_address,
        tables=table_list
    )