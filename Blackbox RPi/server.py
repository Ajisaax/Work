import paho.mqtt.client as client
import paho.mqtt.publish as publish
import json
import serial
import psycopg2
import numpy as np
import random
import threading
global BBSER
global conn
global tpmsId
global ser
global serverIP
serverIP = '52.161.94.109'
ser = None
BBSER = BB0001
tpmsId = None
conn = psycopg2.connect(user="root",password="root",database="blackbox",host="db")

def getPaired(typeDev):
    sql = """SELECT * FROM pairedDev WHERE type = '%s'"""%(typeDev)
    cur = conn.cursor()
    conn.execute(sql)
    val = cur.fetchall()
    val = np.array(val)
    return val


def initialize():
    try:        
        tpmsId = getPaired('tpms')[:,[1,2]]
        nodeId = getPaired('node')[:,[1,2]]
        return tpmsId, nodeId
    except Exception as e:
        print(e)

def insertData(serialId, data, devId):
    try:
        sql = """INSERT INTO dataDev(serialId,value,devId) VALUES('%s','%s','%s')"""%(serialId, data, devId)
        cur = conn.cursor()
        cur.execute(sql)
        conn.commit()
    except Exception as e:
        print(e)

def publishData(topic,data):
    global serverIP
    publish.single(topic,data,hostname=serverIP,user='ridho',password='123')


def filterTPMS(mosq,obj,msg):
    global tpmsId
    global BBSER
    dataTPMS = str(msg.payload,'utf-8')
    dataTPMS = json.loads(dataTPMS)
    tpmsID = str(dataTPMS['id'])
    if tpmsID in tpmsId.keys():
        dataProcess = {
        'serialId':tpmsId[tpmsID],
        'devId':tpmsID,
        'TEMP':dataTPMS['temperature_C'],
        'PRESS':dataTPMS['pressure_kPa'],
        'type':'TPMS'}
        insertData(tpmsId[tpmsID],json.dumps(dataProcess),tpmsID)
        sendMessage = {'BBSer':BBSER, 'Node':json.dumps(dataProcess)}
        publishData('/node/TPMS/',json.dumps(sendMessage))

def engineControl(mosq,obj,msg):
    global ser
    global BBSER
    dataEngine = str(msg.payload,'utf-8')
    dataEngine = json.loads(dataEngine)
    if dataEngine["BBSER"] == BBSER:
        value = dataEngine["Node"]
        engineState = value["ENG"]
        randomValue = str(random.randint(100,999))+str(engineState)
        sendMessage = {'id':2,'serialId':value['serialId'],'ENG':randomValue}
        ser.write(b"%a"%(sendMessage))



def geTPMS():
    global serverIP
    try: 
        subsInternet = client.Client("node"+str(random.randint(1000,9999)))
        subsInternet.message_callback_add("/engineControl",engineControl)
        subsInternet.connect(serverIP,1883,60)
        subsInternet.subscribe("/#",0)
        subs = client.Client("TPMSsubs")
        subs.message_callback_add("/rtl_433",filterTPMS)
        subs.connect("mqtt",1883,60)
        subs.subscribe("/#",0)
        subs.loop_forever()
    except Exception as e:
        print(e)


def main():
    global tpmsId
    global BBSER
    global ser
    tpmsId,nodeId = initialize()
    tpmsId=dict(zip(tpmsId[:,0],tpmsId[:,1]))
    nodeId=dict(zip(nodeId[:,0],nodeId[:,1]))
    ser = serial.Serial(port='/dev/nodemcu',baudrate=9600)
    while True:
        serialData = ser.readline().decode('utf-8').rstrip()
        dataSensor = json.loads(serialData)
        devId = dataSensor["devId"]
        serialId = dataSensor["serialId"]
        if devId == nodeId.keys() and serialId == nodeId[devId]:
            if dataSensor["id"] == 1:
                nodeData = dataSensor['message'].split(',')
                dataProses = {'type':'FMS',
                'serialId':serialId,
                'devId':devId,
                'FS':nodeData[0],
                'CS':nodeData[1]
                }
                insertData(serialId,json.dumps(dataProses),devId)
                sendMessage = {'BBSer':BBSER,'Node':json.dumps(dataProses)}
                publishData('/node/FMS/',json.dumps(sendMessage))
            if dataSensor["id"] == 2:
                nodeData = dataSensor['message'].split(',')
                dataProses = {'type':'PMS',
                'serialId':serialId,
                'devId':devId,
                'LAT':nodeData[0],
                'LON':nodeData[1],
                'DRI':nodeData[2],
                'PB':nodeData[3],
                'GX':nodeData[4],
                'GY':nodeData[5],
                'GZ':nodeData[6],
                'ENG':nodeData[7]                
                }
                insertData(serialId,json.dumps(dataProses),devId)
                sendMessage = {'BBSer':BBSER,'Node':json.dumps(dataProses)}
                publishData('/node/PMS',json.dumps(sendMessage))
