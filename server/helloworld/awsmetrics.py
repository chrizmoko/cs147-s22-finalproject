from datetime import datetime, date
from pytz import timezone
import time

import boto3


cloudwatch = boto3.client("cloudwatch", region_name='us-west-1')
logs = boto3.client('logs', region_name='us-west-1')

sequenceToken = None
NUM_RETRIES = 3
LOG_GROUP_NAME = 'messageLogs'
LOG_STREAM_NAME = 'serverLogs'
TWO_WEEKS_IN_SECS = 1209600
TIMEOUT_IN_SECONDS = 3

def retrieve_metric_data(senderId, limit):
    try:
        response = logs.start_query(
            logGroupName=LOG_GROUP_NAME,
            startTime=get_current_time_in_sec()-TWO_WEEKS_IN_SECS,
            endTime=get_current_time_in_sec(),
            queryString="fields @timestamp, @message " +
                        "| filter @message like 'sender={},' ".format(str(senderId))+
                        "| sort @timestamp desc",
            limit=limit
        )
        if "queryId" not in response:
            return response
        else:
            queryId = response["queryId"]
            response = logs.get_query_results(
                queryId=queryId
            )
            if "status" in response:
                start_time = get_current_time_in_sec()
                current_time = get_current_time_in_sec()
                while response["status"] != "Complete" or current_time - start_time > TIMEOUT_IN_SECONDS:
                    response = logs.get_query_results(
                        queryId=queryId
                    )
                    time.sleep(0.5)
                    current_time = get_current_time_in_sec()
            return response
    except Exception as e:
        raise e

def format_heatmap_metrics(unformatted_metrics):
    # makes sure the input is not empty
    # Output:
    # {Sun: [0,0,0,0,0,1,0,0,0,0,0,0,0]}
    days_of_week = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
    unformatted_metrics = unformatted_metrics["results"]
    formatted_metrics = {}
    for day in days_of_week:
        formatted_metrics[day] = [0 for _ in range(24)]

    if len(unformatted_metrics) > 0:
        for metric in unformatted_metrics:
            for entry in metric:
                if entry["field"] == "@timestamp":
                    timestamp = entry["value"]
                    date = datetime.strptime(timestamp, '%Y-%m-%d %H:%M:%S.%f').astimezone(timezone('US/Pacific'))
                    day = str(date.strftime('%a'))
                    hour = int(date.strftime('%H'))
                    formatted_metrics[day][hour] += 1
    return formatted_metrics

def put_metrics(metricValue, metricName, senderId):
    try:
        response = cloudwatch.put_metric_data(
            Namespace = 'finalProjectMetrics',
            MetricData = [
            {
                'MetricName': metricName,
                'Dimensions': [
                    {
                        'Name': 'senderId',
                        'Value': senderId
                    },
                ],
                'Value': float(metricValue),
                'Timestamp': datetime.now()
,
            },
        ])
        print(response)
    except Exception as e:
        raise e

def put_logs(senderId, message, retryAttempts, sequenceToken):
    if retryAttempts == 0:
        print("Log message can not be sent")
        return
    try:
        logMessage = "sender=" + str(senderId) + ",message=" + message
        logPackage = {
            "logGroupName":LOG_GROUP_NAME,
            "logStreamName":LOG_STREAM_NAME,
            "logEvents":[
                {
                    'timestamp': get_current_time_in_millis(),
                    'message': logMessage
                },
            ],
        }
        if sequenceToken is not None:
            logPackage["sequenceToken"] = sequenceToken
        response = logs.put_log_events(**logPackage)
        return response['nextSequenceToken']
    except logs.exceptions.InvalidSequenceTokenException as e:
        sequenceToken = e.response["Error"]["Message"].split("is: ")[-1]
        return put_logs(senderId, message, retryAttempts-1, sequenceToken)
    except logs.exceptions.DataAlreadyAcceptedException as e:
        return
    except Exception as e:
        raise e

def get_current_time_in_millis():
    return int((datetime.utcnow() - datetime(1970, 1, 1)).total_seconds() * 1000)

def get_current_time_in_sec():
    return int((datetime.utcnow() - datetime(1970, 1, 1)).total_seconds())