import boto3  # http messaging
import json # json text builder/reader
import time

clientIOT = boto3.client('iot-data', region_name='us-east-1')

NXP_module_name_1 = 'KitchenThing'
NXP_module_name_2 = 'BedroomThing'

# --------------- Helpers that build all of the responses ----------------------

def build_speechlet_response(response_message, reprompt_text, should_end_session):
    return {
        'outputSpeech': {
            'type': 'PlainText',
            'text': response_message
        },
        'reprompt': {
            'outputSpeech': {
                'type': 'PlainText',
                'text': reprompt_text
            }
        },
        'shouldEndSession': should_end_session
    } 
    

def build_response(session_attributes, speechlet_response):
    return {
        'version': '1.0',
        'sessionAttributes': session_attributes,
        'response': speechlet_response
    }

# ---------------- Updating and reading Device's Shadow ----------------

def update_shadow(device, mypayload):
    clientIOT.update_thing_shadow(
        thingName = device,
        payload = mypayload
        )
    return()

def read_from_shadow(device):
    return(clientIOT.get_thing_shadow(                    
        thingName = device
        ))

# ----------------- Functions ------------

def no_communication_warning(room): # gives a warning when targeted device is not online
    
    should_end_session = True
    reprompt_text = None
    response_message = 'It was not possible to reach your ' + room + \
                        '. Please, try again.'
    return(build_response({}, build_speechlet_response(response_message, reprompt_text, should_end_session)))
    
    
def ask_for_room(session_attributes, request):  # ask for a specific room, if not set already
    incomplete_attributes = session_attributes
    should_end_session = False
    
    speech_output = 'Do you mean in the kitchen?, '\
                        ' or in the bedroom? Or both'


    reprompt_text = 'I do not know which room you chose, '  \
                        'Do you mean the kitchen?' \
                        'Or the bedroom? Or both?'

    return build_response(incomplete_attributes, build_speechlet_response(
        speech_output, reprompt_text, should_end_session))


def send_LED_message(session_attributes, session):
    
    room = session_attributes['room']
    action = session_attributes['action']
    
    if room == 'kitchen':
        addressed_device = NXP_module_name_1
    elif room == 'bedroom':
        addressed_device = NXP_module_name_2
    elif room == 'both':
        return(send_LED_message_to_both(action))
    
    if check_communication(addressed_device) == 'warning':          # before going on checks if device is online
        return(no_communication_warning(room))
    
    should_end_session = True
    
    if action == 'on':
        desired_state = 1
    if action == 'off':
        desired_state = 0 
    
    time.sleep(1)
    
    stato = {"state" : { "desired" : { "LEDstate": desired_state }}}            # if commm. is ok sends message to NXP module
    mypayload = json.dumps(stato)
    update_shadow(addressed_device, mypayload)
    
    time.sleep(1)
    
    shadow = read_from_shadow(addressed_device)                   #read LED state after sleeping 2 sec
    streamingBody = shadow["payload"]
    jsonState = json.loads(streamingBody.read())
    state_of_LED = jsonState['state']['reported']['LEDstate']
    
    if state_of_LED == 1:
        return_message = 'the LED in your ' + room + ' is on'
    if state_of_LED == 0:
        return_message = 'the LED in your ' + room + ' is off'
    
    answer_for_alexa = build_speechlet_response(return_message, None, should_end_session)
    return(build_response(session_attributes, answer_for_alexa))


def send_LED_message_to_both(action):
    
    should_end_session = True
    
    if action == 'on':
        desired_state = 1
    if action == 'off':
        desired_state = 0
    if check_communication(NXP_module_name_1) == 'warning':          # before going on checks if device is online
        return(no_communication_warning('kitchen'))
    if check_communication(NXP_module_name_2) == 'warning':          # before going on checks if device is online
        return(no_communication_warning('bedroom'))
    
    time.sleep(1)
    
    stato = {"state" : { "desired" : { "LEDstate": desired_state }}}            
    mypayload = json.dumps(stato)
    update_shadow(NXP_module_name_1, mypayload)
    update_shadow(NXP_module_name_2, mypayload)
           
    time.sleep(1)
    
    shadow = read_from_shadow(NXP_module_name_1)                  #read LED state of kitchen
    streamingBody = shadow["payload"]
    jsonState = json.loads(streamingBody.read())
    state_of_LED_1 = jsonState['state']['reported']['LEDstate']
    
    shadow = read_from_shadow(NXP_module_name_2)                    #read LED state of bedroom
    streamingBody = shadow["payload"]
    jsonState = json.loads(streamingBody.read())
    state_of_LED_2 = jsonState['state']['reported']['LEDstate']
    
    if (state_of_LED_1 == 1) & (state_of_LED_2 == 1):
        return_message = 'the LED in your kitchen and in your bedroom are on'
    if (state_of_LED_1 == 0) & (state_of_LED_2 == 0):
        return_message = 'the LED in your kitchen and in your bedroom are off'
    
    answer_for_alexa = build_speechlet_response(return_message, None, should_end_session)
    return(build_response({}, answer_for_alexa))
    

def send_ReadTemp_Message(session_attributes, session):
    
    should_end_session = True
    
    if session_attributes['room'] != "":
        room = session_attributes['room']
    
    else:
        room  = session['attributes']['room']
    
    if room == 'kitchen':
        addressed_device = NXP_module_name_1
    elif room == 'bedroom':
        addressed_device = NXP_module_name_2
    elif room == 'both':
        return(send_ReadTemp_Message_to_both())
    
    if check_communication(addressed_device) == 'warning':          # before going on checks if device is online
        return(no_communication_warning(room))
    
    time.sleep(1)
    
    shadow = read_from_shadow(addressed_device)
    streamingBody = shadow["payload"]   
    jsonState = json.loads(streamingBody.read())
    temperature = str(jsonState['state']['reported']['Temp'])
    
    if room == 'kitchen':
        return_message = 'The temperature in your ' + room + 'is: ' + temperature + ' degrees Celsius. Enjoy your meal!'   
    elif room == 'bedroom':
        return_message = 'The temperature in your ' + room + 'is: ' + temperature + ' degrees Celsius. Have a good night.' 
    
    session_attributes = {}
    answer_for_alexa = build_speechlet_response(return_message, None, should_end_session)
    return(build_response(session_attributes, answer_for_alexa))

# this function is called when user asks for both the modules' temperatures
def send_ReadTemp_Message_to_both():
    
    should_end_session = True #
    
    if check_communication(NXP_module_name_1) == 'warning':          # before going on checks if device is online
        return(no_communication_warning('kitchen'))
    if check_communication(NXP_module_name_2) == 'warning':          # before going on checks if device is online
        return(no_communication_warning('bedroom'))

    time.sleep(1)
    
    shadow = read_from_shadow(NXP_module_name_1)
    streamingBody = shadow["payload"]
    jsonState = json.loads(streamingBody.read())
    temperature_kitchen = str(jsonState['state']['reported']['Temp'])
    
    shadow = read_from_shadow(NXP_module_name_2)
    streamingBody = shadow["payload"]
    jsonState = json.loads(streamingBody.read())
    temperature_bedroom = str(jsonState['state']['reported']['Temp'])
    
    return_message = 'The temperature in your kitchen is: ' + temperature_kitchen + 'degrees Celsius.' + \
                    ' The temperature in your bedroom is: ' + temperature_bedroom + 'degrees Celsius.'
    
    session_attributes = {}
    answer_for_alexa = build_speechlet_response(return_message, None, should_end_session)
    return(build_response(session_attributes, answer_for_alexa))


# this function checks the communication state of the NXP module
# if module is online nothing happens
# if module is offline returns a warning
def check_communication(addressed_device):
    
    communication = 'ok' # set dafault communication state to ok
    
    shadow = read_from_shadow(addressed_device)      # get thing shadow
    streamingBody = shadow["payload"]
    jsonState = json.loads(streamingBody.read())
    communication_state = jsonState['state']['reported']['ConnState'] 

    if communication_state == 0:
        switch_state = 1
    else:
        switch_state = 0
        
    stato = {"state" : {"desired": { "ConnState" : switch_state}}} # publish a different desired state on shadow
    mypayload = json.dumps(stato)
    update_shadow(addressed_device, mypayload)
    
    # gget the updated shadow communication state (3 iterations)
    for i in range(3):
        shadow = read_from_shadow(addressed_device)     # get shadow
        streamingBody = shadow["payload"]
        jsonState = json.loads(streamingBody.read())
        communication_state = jsonState['state']['reported']['ConnState'] 
        if communication_state != switch_state:
            i = i+1
            time.sleep(1)
        else:
            i = 5
    
    # if module is offline set communication state to warning        
    if communication_state != switch_state:
        communication = 'warning'
    
    # returns communication state
    return(communication)
    
# ------------------------- EVENTS --------------------------

# this is called when user wants to interact with module's LED 
def manage_LED_request(request, session):
    
    # get desired LED state (on | off)
    action = request['intent']['slots']['LEDMessage']['value']
    
    session_attributes_init = {"request": request['intent']['name'], 
    "action": action, "room": ""}
    
    # if no room is selected asks for room
    if 'value' not in request['intent']['slots']['ROOMselection']:
        session_attributes = session_attributes_init
        return(ask_for_room(session_attributes, request))
    
    # if room is selected launch set LED state request
    else:
        session_attributes_init['room'] = request['intent']['slots']['ROOMselection']['value']
        session_attributes = session_attributes_init
        return(send_LED_message(session_attributes, session))

# -- end of function

# this is called when user wants to get module's temperature
def manage_Temp_request(request, session):
    
    session_attributes_init = {"request": request['intent']['name'], "action": "read_temp", "room": ""}
    
    # if no room is selected asks for room
    if 'value' not in request['intent']['slots']['ROOMselection']:
        session_attributes = session_attributes_init
        return(ask_for_room(session_attributes, request))
    
    # if room is selected launch get temperature request
    else:
        session_attributes_init['room'] = request['intent']['slots']['ROOMselection']['value']
        session_attributes = session_attributes_init
        return(send_ReadTemp_Message(session_attributes, session))

# -- end of function
    
# this is called when user declares which room to interact with
def manage_Room_request(request, session):
    
    # set the selected room
    room = request['intent']['slots']['ROOMselection']['value']
    
    # calling the LED interaction function
    if session['attributes']['request'] == 'LEDIntent':
        session_attributes = session['attributes']
        session_attributes['room'] = room
        return(send_LED_message(session_attributes, session))
    
    # calling the T sensor interaction function
    elif session['attributes']['request'] == 'TempIntent':
        session_attributes = {'room': room }
        return(send_ReadTemp_Message(session_attributes, session))

# -- end of function

# ------------------------- MAIN ----------------------------

def lambda_handler(event, context):
    
    # request_type comes from Alexa (three possible cases, see below)
    request_type = event['request']['intent']['name'] # could be "LEDIntent" or "TempIntent" or "RoomIntent"
    
    # case 1: user wants to interact with module's LED
    # manage interact with LED request
    if request_type == 'LEDIntent':
        return(manage_LED_request(event['request'], event['session']))
    
    # case 2: user wants to get module's temperature
    # manage get temperature request
    if request_type =='TempIntent':
        return(manage_Temp_request(event['request'], event['session']))
    
    # case 3: room selection (i.e. module selection)
    # manage room selection request
    if request_type == 'RoomIntent':
        return(manage_Room_request(event['request'], event['session']))