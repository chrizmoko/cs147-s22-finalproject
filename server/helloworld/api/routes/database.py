from flask import Blueprint

from helloworld import database


blueprint: Blueprint = Blueprint('database', __name__, url_prefix='/database')


@blueprint.route('/', methods=['GET', 'POST'])
def get_all():
    """Response format:
    
        {
            devices (array)
            [
                {
                    macAddress (string)
                    unreadMessages (array)
                    [
                        {
                            macAddress (string) ; of the sender
                            content (string)
                            time (string) 
                        }
                    ]
                    username (string)
                }
            ]        
        }
    """
    device_list = [device for device in database.to_dict().values()]
    return {'devices': device_list}

