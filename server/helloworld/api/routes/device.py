from flask import Blueprint, request
from http.client import BAD_REQUEST, FORBIDDEN
from typing import List

from helloworld.api.error import ErrorType, ErrorBuilder, RequestErrorBuilder
from helloworld import database
from helloworld import awsmetrics


blueprint: Blueprint = Blueprint('device', __name__, url_prefix='/device')
message_blueprint: Blueprint = Blueprint('message', __name__, 
                                         url_prefix='/message')


@blueprint.route('/register', methods=['POST'])
def register():
    """Required arguments:
    
        macAddress -- the MAC address of the device
    
    Optional arguments:

        username -- the username of the device; if not specified the macAddress
            is used
    """

    mac_address = request.form.get('macAddress')
    username = request.form.get('username')

    error_found = False
    error_builder = RequestErrorBuilder()
    
    if mac_address == None:
        error_builder.add_argument_required('macAddress')
        error_found = True
    
    if error_found:
        return error_builder.build(), BAD_REQUEST
    
    if username == None:
        username = mac_address
    
    database.add_user(mac_address, username)

    return {}


@blueprint.route('/unregister', methods=['POST'])
def unregister():
    """Required arguments:
    
        macAddress -- the MAC address of the device
    """
    mac_address = request.form.get('macAddress')

    error_found = False
    error_builder = RequestErrorBuilder()
    
    if mac_address == None:
        error_builder.add_argument_required('macAddress')
        error_found = True
    
    if error_found:
        return error_builder.build(), BAD_REQUEST
    

    database.remove_user(mac_address)

    return {}


@message_blueprint.route('/receive', methods=['POST'])
def receive():
    """Required arguments:
    
        macAddress -- the MAC address of the device
        message -- the message sent by the device
    """
    mac_address = request.form.get('macAddress')
    message = request.form.get('message')

    error_found = False
    error_builder = RequestErrorBuilder()
    
    if mac_address == None:
        error_builder.add_argument_required('macAddress')
        error_found = True
    
    if message == None:
        error_builder.add_argument_required('username')
        error_found = True
    
    if error_found:
        return error_builder.build(), BAD_REQUEST
    
    def add_unread_message(user: database.UserDevice) -> None:
        if user.mac_address() != mac_address:
            user.add_pending_message(message, mac_address)
    
    database.for_each_user(add_unread_message)

    awsmetrics.put_metrics(1, "message", mac_address)
    awsmetrics.put_logs(mac_address, message, awsmetrics.NUM_RETRIES, awsmetrics.sequenceToken)

    return {}


@message_blueprint.route('/pending/count', methods=['POST'])
def count_pending():
    """Required arguments:
    
        macAddress (string) -- the MAC address of the device
    
    Response format:
    
        {
            count (integer)        
        }
    """
    mac_address = request.form.get('macAddress')

    error_found = False
    error_builder = RequestErrorBuilder()

    if error_found == None:
        error_builder.add_argument_required('macAddress')
        error_found = True
    
    if error_found:
        return error_builder.build(), BAD_REQUEST
    
    try:
        user: database.UserDevice = database.get_user(mac_address)
        count: int = user.count_pending_messages()
    except database.DatabaseException:
        error_builder = ErrorBuilder(
            ErrorType.UNAUTHORIZED_REQUEST,
            'Device is not visible to the server'
        )
        return error_builder.build(), FORBIDDEN

    return {'count': count}


@message_blueprint.route('/pending/get', methods=['POST'])
def get_pending():
    """Required arguments:
    
            macAddress -- the MAC address of the device
            limit -- the maximum amount of messages to send back as a positive
                integer; the actual amount may be less
    
    Response format:
    
        {
            count (integer)
            messages (array)
            [
                {
                    macAddress (string) ; of the sender
                    content (string)
                    time (string) 
                }
            ]
        }
    """
    mac_address = request.form.get('macAddress')
    limit = request.form.get('limit')

    error_found = None
    error_builder = RequestErrorBuilder()

    if mac_address == None:
        error_builder.add_argument_required('macAddress')
        error_found = True
    
    if limit == None:
        error_builder.add_argument_required('limit')
        error_found = True
    
    if not limit.isnumeric():
        error_builder.add_invalid_argument_type('limit')
        error_found = True

    limit = int(limit)

    if limit < 0:
        error_builder.add_invalid_argument_value('limit')
        error_found = True
    
    if error_found:
        return error_builder.build(), BAD_REQUEST
    
    try:
        user: database.UserDevice = database.get_user(mac_address)
        messages: List[database.Message] = user.get_pending_messages(limit)
    except database.DatabaseException:
        error_builder = ErrorBuilder(
            ErrorType.UNAUTHORIZED_REQUEST,
            'Device is not visible to the server'
        )
        return error_builder.build(), FORBIDDEN

    message_list = [message.as_dict() for message in messages]
    return {'count': len(message_list), 'messages': message_list}

@message_blueprint.route('/metric/retrieve', methods=['GET'])
def retrieve_metrics():
    mac_address = request.args.get('macAddress')
    limit = request.args.get('limit')

    error_found = None
    error_builder = RequestErrorBuilder()

    print(mac_address, limit)
    if mac_address == None:
        error_builder.add_argument_required('macAddress')
        error_found = True

    if limit == None:
        error_builder.add_argument_required('limit')
        error_found = True
    
    if not limit.isnumeric():
        error_builder.add_invalid_argument_type('limit')
        error_found = True
    
    limit = int(limit)

    if limit < 0:
        error_builder.add_invalid_argument_value('limit')
        error_found = True

    if error_found:
        return error_builder.build(), BAD_REQUEST

    return retrieve_metrics_internal(mac_address, limit)


def retrieve_metrics_internal(mac_address: str, limit: int):
    unformatted_metrics = awsmetrics.retrieve_metric_data(mac_address, limit)
    metrics = awsmetrics.format_heatmap_metrics(unformatted_metrics)
    return metrics



blueprint.register_blueprint(message_blueprint)