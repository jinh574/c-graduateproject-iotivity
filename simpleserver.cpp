// 16.09.02 simpleserver test
// 16.09.28 basic method
// 16.10.05 mqtt publish
// 16.10.10 vector add
// 16.10.13 mqtt subscribe
// 16.10.31 camera test

#include <functional>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <cctype>
#include <chrono>
#include <vector>
#include <uuid/uuid.h>

#include "OCPlatform.h"
#include "OCApi.h"

#include "json/json.h"
#include "mqtt/async_client.h"

#define UUID_SIZE	36

using namespace OC;
using namespace std;
namespace PH = std::placeholders;

static const char* SVR_DB_FILE_NAME = "./oic_svr_db_server.dat";
int gObservation = 0;
bool isListOfObservers = false;
void *ChangeValueRepresentation (void *param);

/* sensor number */
int number = 0;

/* Resrouce vector */
class Resource;
std::vector<Resource *> v;

/* controller */
std::string controllerID;
std::string controllerIP = "192.168.0.38";
std::string controllerNAME = "controllerPI";
std::string adminNAME;
std::string adminTEL;
std::string description;
std::string delete_sensor1;
std::string delete_sensor2;
int isDelete = 0;
int isFire = 0;
int pNumber = 0;
//char **p;
std::vector<std::string> vs;
int *q;

/* mqtt test */
const std::string MQTT_ADDRESS("tcp://203.252.146.154:1883");
const std::string MQTT_CLIENTID("ControllerPi1");
const std::string MQTT_CLIENTID2("ControllerPi2");
const std::string TOPIC("iotivity");
const std::string TOPIC2("iotivityCtrl");
const int QOS = 1;
const long TIMEOUT = 10000L;

/* A callback class for use with the main MQTT client. */
class callback : public virtual mqtt::callback
{
public:
	virtual void connection_lost(const std::string& cause) {
		std::cout << "\nConnection lost" << std::endl;
		if(!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;
	}
	virtual void message_arrived(const std::string& topic, mqtt::const_message_ptr msg) {}
	virtual void delivery_complete(mqtt::idelivery_token_ptr tok) {
		std::cout << "Delivery complete for token: " << (tok ? tok->get_message_id() : -1) << std::endl;
	}		
};

/* A base action listner. */
class action_listener : public virtual mqtt::iaction_listener
{
protected:
	virtual void on_failure(const mqtt::itoken& tok) {
		std::cout << "\n\tListener: Failure on token: " << tok.get_message_id() << std::endl;
	}
	virtual void on_success(const mqtt::itoken& tok) {
		std::cout << "\n\tListener: Success on token: " <<tok.get_message_id() << std::endl;
	}
};

/* A derived action listener for publish events. */
class delivery_action_listener : public action_listener
{
	bool done_;

	virtual void on_failure(const mqtt::itoken& tok) {
		action_listener::on_failure(tok);
		done_ = true;
	}
	virtual void on_success(const mqtt::itoken& tok) {
		action_listener::on_success(tok);
		done_ = true;
	}

public:
	delivery_action_listener() : done_(false) {}
	bool is_done() const { return done_; }
};

/////////////////////////////////////////////////////////////////

/* mqtt Subscriber action listener */
class s_action_listener : public virtual mqtt::iaction_listener
{
	std::string name_;

	virtual void on_failure(const mqtt::itoken& tok) {
		std::cout << name_ << " failure";
		if (tok.get_message_id() != 0)
			std::cout << " (token: " << tok.get_message_id() << ")" << std::endl;
		std::cout << std::endl;
	}

	virtual void on_success(const mqtt::itoken& tok) {
		if(tok.get_message_id() != 0) {}
		if(!tok.get_topics().empty()) {}
	}
public:
	s_action_listener(const std::string& name) : name_(name) {}
};

class s_callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener
{
	int nretry_;
	mqtt::async_client& cli_;
	s_action_listener& listener_;

	void reconnect() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		mqtt::connect_options connOpts;
		connOpts.set_keep_alive_interval(20);
		connOpts.set_clean_session(true);

		try {
			cli_.connect(connOpts, nullptr, *this);
		}
		catch (const mqtt::exception& exc) {
			std::cerr << "Error: " << exc.what() << std::endl;
			exit(1);
		}
	}

	virtual void on_failure(const mqtt::itoken& tok) {
		std::cout << "Reconnection failed." << std::endl;
		if (++nretry_ > 5)
			exit(1);
		reconnect();
	}

	virtual void on_success(const mqtt::itoken& tok) {
		std::cout << "Reconnection success" << std::endl;
		cli_.subscribe(TOPIC2, QOS, nullptr, listener_);
	}

	virtual void connection_lost(const std::string& cause) {
		std::cout << "\nConnection lost" << std::endl;
		if(!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;

		std::cout << "Reconnecting." << std::endl;
		nretry_ = 0;
		reconnect();
	}

	virtual void message_arrived(const std::string& topic, mqtt::const_message_ptr msg) {
		std::cout << "Message arrived" << std::endl;
		std::cout << "\ttopic: '" << topic << "'" << std::endl;
		std::cout << "\t'" << msg->to_str() << std::endl;
		
		std::string json_temp = msg->to_str();
		Json::Value root;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(json_temp, root, false);
		if(not parsedSuccess)
		{
			std::cout << "Failed to parse JSON" << std::endl;
		}
		
		int code = root["code"].asInt();
		std::string tempString;
		tempString = root["data"]["controller_id"].asString();
		if(tempString == controllerID){
		if(code == 0) // information change
		{
			const Json::Value data = root["data"];
			tempString = root["data"]["controller_name"].asString();
			if(tempString.length()) controllerNAME = tempString;
			tempString = root["data"]["admin_name"].asString();
			if(tempString.length()) adminNAME = tempString;
			tempString = root["data"]["admin_tel"].asString();
			if(tempString.length()) adminTEL = tempString;
			tempString = root["data"]["description"].asString();
			if(tempString.length()) description = tempString;
		}
		else if(code == 1) // controller alarm on
		{
			const Json::Value data = root["data"];
			isFire = root["data"]["type"].asInt();
		}
		else if(code == 2) // controller alarm off
		{
			isFire = 0;
		}
		else if(code == 3) // count people
		{
			const Json::Value data = root["data"];
			pNumber = data.size();
			q = new int[pNumber];
			vs.clear();
			for(int index = 0; index < pNumber; ++index)
			{
				q[index] = data[index]["light_state"].asInt();
				vs.push_back(data[index]["sensor_id"].asString());
			}
			std::vector<std::string>::iterator vi;
			for(vi=vs.begin(); vi!=vs.end(); vi++)
			{
				cout << "UUID : " <<*vi << endl;
			}
		}
		else // delete sensor
		{
			const Json::Value data = root["data"];
			delete_sensor1 = root["data"]["sensor_id"].asString();
			delete_sensor2 = root["data"]["sensor_uri"].asString();
			isDelete = 1;
		}
		}
	}

	virtual void delivery_complete(mqtt::idelivery_token_ptr token) {}

public:
	s_callback(mqtt::async_client& cli, s_action_listener& listener) : cli_(cli), listener_(listener) {}
};

///////////////////////////////////////////////////////////////////////////////
class Resource
{
public:
	std::string m_name;
	std::string m_sensorUri;
	int light_state;
	int light_power;
	int gas_state;
	int gas_efflux;
	int temp_state;
	int humi_state;
	double temper;
	double humi;
	int flame_state;
	int flame_power;
	int fire_alarm;
	int time_stp;
	std::string person;

	Resource(std::string _m_name, std::string _m_sensorUri, int _light_state, int _light_power, int _gas_state, int _gas_efflux, int _temp_state, int _humi_state, double _temper, double _humi, int _flame_state, int _flame_power, int _fire_alarm, int _time_stp, std::string _person)
	{
		m_name = _m_name; m_sensorUri = _m_sensorUri;
		light_state = _light_state; light_power = _light_power;
		gas_state = _gas_state; gas_efflux = _gas_efflux;
		temp_state = _temp_state; humi_state = _humi_state; temper = _temper; humi = _humi;
		flame_state = _flame_state; flame_power = _flame_power;
		fire_alarm = _fire_alarm, time_stp = _time_stp; person = _person;
	}
};

class SensorResource
{

public:
	std::string m_name;
	int light_state;
	int light_power;
	int gas_state;
	int gas_efflux;
	double temper;
	double humi;
	int temp_state;
	int humi_state;
	int flame_state;
	int flame_power;
	int fire_alarm;
	int time_stp;
	std::string json_info;

	std::string m_sensorUri;
	OCResourceHandle m_resourceHandle;
	OCRepresentation rep;
	ObservationIds m_interestedObservers;

public:
	SensorResource() : m_name("my sensor"), light_state(0), light_power(0),
						gas_state(0), gas_efflux(0), temper(0.0), humi(0.0), 
						temp_state(0), humi_state(0), flame_state(0), flame_power(0), fire_alarm(0), time_stp(0), 
						json_info("[]"), m_sensorUri("/a/sensor0"), m_resourceHandle(nullptr) {
		rep.setUri(m_sensorUri);
		rep.setValue("m_name", m_name);
		rep.setValue("light_state", light_state);
		rep.setValue("light_power", light_power);
		rep.setValue("gas_state", gas_state);
		rep.setValue("gas_efflux", gas_efflux);
		rep.setValue("temper", temper);
		rep.setValue("humi", humi);
		rep.setValue("temp_state", temp_state);
		rep.setValue("humi_state", humi_state);
		rep.setValue("flame_state", flame_state);
		rep.setValue("flame_power", flame_power);
		rep.setValue("fire_alarm", fire_alarm);
		rep.setValue("time_stp", time_stp);
		rep.setValue("json_info", json_info);
	}

	SensorResource(std::string _m_sensorUri) : m_name("my sensor"), light_state(0), light_power(0),
						gas_state(0), gas_efflux(0), temper(0.0), humi(0.0), 
						temp_state(0), humi_state(0), flame_state(0), flame_power(0), fire_alarm(0), time_stp(0), 
						json_info("[]"), m_sensorUri(_m_sensorUri), m_resourceHandle(nullptr) {
		rep.setUri(_m_sensorUri);
		rep.setValue("m_name", m_name);
		rep.setValue("light_state", light_state);
		rep.setValue("light_power", light_power);
		rep.setValue("gas_state", gas_state);
		rep.setValue("gas_efflux", gas_efflux);
		rep.setValue("temper", temper);
		rep.setValue("humi", humi);
		rep.setValue("temp_state", temp_state);
		rep.setValue("humi_state", humi_state);
		rep.setValue("flame_state", flame_state);
		rep.setValue("flame_power", flame_power);
		rep.setValue("fire_alarm", fire_alarm);
		rep.setValue("time_stp", time_stp);
		rep.setValue("json_info", json_info);
	}

	void createResource()
	{
		std::string resourceURI = m_sensorUri;
		std::string resourceTypeName = "core.sensor";
		std::string resourceInterface = BATCH_INTERFACE;

		uint8_t resourceProperty;
		resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;
		EntityHandler cb = std::bind(&SensorResource::entityHandler, this, PH::_1);

		OCStackResult result = OCPlatform::registerResource(
				m_resourceHandle, resourceURI, resourceTypeName, resourceInterface, cb, resourceProperty);

		v.push_back(new Resource("my sensor", "/a/sensor0", 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, 0, "[]"));

		if (OC_STACK_OK != result)
		{
			std::cout << "Resource creation was unsuccessful\n";
		}
	}

	OCStackResult createResource1()
	{
		std::string resourceURI = "/a/sensor"+to_string(number);
		std::string resourceTypeName = "core.sensor";
		std::string resourceInterface = BATCH_INTERFACE;

		uint8_t resourceProperty;
		resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;
		EntityHandler cb = std::bind(&SensorResource::entityHandler, this, PH::_1);

		OCResourceHandle resHandle;

		OCStackResult result = OCPlatform::registerResource(
				resHandle, resourceURI, resourceTypeName, resourceInterface, cb, resourceProperty);

		v.push_back(new Resource("my sensor", resourceURI, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, 0, "[]"));

		if (OC_STACK_OK != result)
		{
			std::cout << "Resource creation was unsuccessful\n";
		}

		return result;
	}

	OCResourceHandle getHandle()
	{
		return m_resourceHandle;
	}

	void put(OCRepresentation& rep)
	{
		try {
			rep.getValue("m_name", m_name);
			std::cout << "\t\t\t\turi : " << m_sensorUri << std::endl;
			if (rep.getValue("light_state", light_state) && rep.getValue("light_power", light_power))
			{
				std::cout << "\t\t\t\t" << "light state : " << light_state << " // light power : " << light_power << std::endl;
			}
			else
			{
				std::cout << "\t\t\t\t" << "light value not found in the representation" << std::endl;
			}

			if (rep.getValue("gas_state", gas_state) && rep.getValue("gas_efflux", gas_efflux))
			{
				std::cout << "\t\t\t\t" << "gas state : " << gas_state << " // gas efflux : " << gas_efflux << std::endl;
			}
			else
			{
				std::cout << "\t\t\t\t" << "gas value not found in the representation" << std::endl;
			}

			if (rep.getValue("temper", temper) && rep.getValue("humi", humi) && rep.getValue("humi_state", humi_state) && rep.getValue("temp_state", temp_state))
			{
				std::cout << "\t\t\t\t" << "temperature state : " << temp_state << " // temperature : " << temper << " // humidity state : " << humi_state << " // humidity : " << humi << std::endl;
			}
			else
			{
				std::cout << "\t\t\t\t" << "Temp & Humid value not found in the representation" << std::endl;
			}

			if (rep.getValue("flame_state", flame_state) && rep.getValue("flame_power", flame_power))
			{
				std::cout << "\t\t\t\t" << "flame state : " << flame_state << " // flame power : " << flame_power << std::endl;
			}
			else
			{
				std::cout << "\t\t\t\t" << "flame value not found in the representation" << std::endl;
			}
			
			if (rep.getValue("time_stp", time_stp))
			{
				std::cout << "\t\t\t\t" << "time stamp : " << time_stp << std::endl;
			}
			else
			{
				std::cout << "\t\t\t\t" << "time stamp not found in the representation" << std::endl;
			}

			if (rep.getValue("json_info", json_info))
			{
				if(json_info=="")
				{
					json_info = "[]";
				}
				else
				{
					std::cout << "\t\t\t\t" << "json_info : '" << json_info << "'" <<std::endl;
				}
			}
			else
			{
				std::cout << "\t\t\t\t" << "json info not found in the representation" << std::endl;
			}
			
			for(unsigned int i = 0; i < v.size(); i++)
			{
				if((v[i]->m_sensorUri).compare(m_sensorUri)==0)
				{
					v[i]->m_name = m_name;
					v[i]->light_state = light_state;
					v[i]->light_power = light_power;
					v[i]->gas_state = gas_state;
					v[i]->gas_efflux = gas_efflux;
					v[i]->temp_state = temp_state;
					v[i]->humi_state = humi_state;
					v[i]->humi = humi;
					v[i]->temper = temper;
					v[i]->flame_state = flame_state;
					v[i]->flame_power = flame_power;
					v[i]->time_stp = time_stp;
					v[i]->person = json_info;
				}
			}
		}
		catch (exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

	OCRepresentation post(OCRepresentation& rep)
	{
		number++;
		SensorResource *mySensor2 = new SensorResource("/a/sensor"+to_string(number));
		if(OC_STACK_OK == mySensor2->createResource1())
		{
			OCRepresentation rep1;
			std::string temp = "/a/sensor"+to_string(number);
			rep1.setValue("createduri", temp);
	
			return rep1;
		}

		else {
			put(rep);
		}
		return get();
	}

	OCRepresentation get()
	{
		if(pNumber)
		{
			sleep(1);
			for(int i=0; i<pNumber; ++i)
			{
				if(vs[i] == m_name)
				{
					cout << "??" << vs[i] << " / " << m_name << " / " << q[i] << endl;
					light_state = q[i];
				}
			}
		}
		fire_alarm = isFire;
		rep.setValue("m_name", m_name);
		rep.setValue("light_state", light_state);
		rep.setValue("light_power", light_power);
		rep.setValue("gas_state", gas_state);
		rep.setValue("gas_efflux", gas_efflux);
		rep.setValue("temper", temper);
		rep.setValue("humi", humi);
		rep.setValue("temp_state", temp_state);
		rep.setValue("humi_state", humi_state);
		rep.setValue("flame_state", flame_state);
		rep.setValue("flame_power", flame_power);
		rep.setValue("fire_alarm", fire_alarm);
		rep.setValue("time_stp", time_stp);
		rep.setValue("json_info", json_info);

		return rep;
	}
	
	void addType(const std::string& type) const
	{
		OCStackResult result = OCPlatform::bindTypeToResource(m_resourceHandle, type);
		if(OC_STACK_OK != result)
		{
			std::cout << "Binding TypeName to Resource was unsuccessful\n" << std::endl;
		}
	}

	void addInterface(const std::string& interface) const
	{
		OCStackResult result = OCPlatform::bindInterfaceToResource(m_resourceHandle, interface);
		if(OC_STACK_OK != result)
		{
			std::cout << "Binding TypeName to Resource was unsuccessful\n" << std::endl;
		}
	}

private:
OCEntityHandlerResult entityHandler(std::shared_ptr<OCResourceRequest> request)
{
	std::cout << "\tIn Server CPP entity handler:" << std::endl;
	OCEntityHandlerResult ehResult = OC_EH_ERROR;
	if(request)
	{
		std::string requestType = request->getRequestType();
		int requestFlag = request->getRequestHandlerFlag();

		if(requestFlag & RequestHandlerFlag::RequestFlag)
		{
			std::cout << "\t\trequestFlag : Request" << std::endl;
			auto pResponse = std::make_shared<OC::OCResourceResponse>();
			pResponse->setRequestHandle(request->getRequestHandle());
			pResponse->setResourceHandle(request->getResourceHandle());

			QueryParamsMap queries = request->getQueryParameters();

			if(!queries.empty())
			{
				std::cout << "\nQuery processing upto entityHandler" << std::endl;
			}
			for(auto it : queries)
			{
				std::cout << "Query key: " << it.first << " value : " << it.second << std::endl;
			}

			if(requestType == "GET")
			{
				std::cout << "\t\t\trequestType : GET" << std::endl;
				pResponse->setResponseResult(OC_EH_OK);
				pResponse->setResourceRepresentation(get());
				if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
				{
					ehResult = OC_EH_OK;
				}
			}
			else if(requestType == "PUT")
			{
				std::cout << "\t\t\trequestType : PUT" << std::endl;

				OCRepresentation rep = request->getResourceRepresentation();

				put(rep);
				pResponse->setErrorCode(200);
				pResponse->setResponseResult(OC_EH_OK);
				pResponse->setResourceRepresentation(get());
				if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
				{
					ehResult = OC_EH_OK;
				}
			}
			else if(requestType == "POST")
			{
				std::cout << "\t\t\trequestType : POST" << std::endl;

				OCRepresentation rep = request->getResourceRepresentation();
				OCRepresentation rep_post = post(rep);
				pResponse->setResourceRepresentation(rep_post);
				pResponse->setErrorCode(200);
				if(rep_post.hasAttribute("createduri"))
				{
					pResponse->setResponseResult(OC_EH_RESOURCE_CREATED);
					
					pResponse->setNewResourceUri(rep_post.getValue<std::string>("createduri"));
					std::string tempUri = rep_post.getValue<std::string>("createduri");
				}
				else
				{
					pResponse->setResponseResult(OC_EH_OK);
				}

				if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
				{
					ehResult = OC_EH_OK;
				}
			}
			else if(requestType == "DELETE")
			{
				std::cout << "Delete request received" << std::endl;
			}
		}

		if(requestFlag & RequestHandlerFlag::ObserverFlag)
		{
			ObservationInfo observationInfo = request->getObservationInfo();
			if(ObserveAction::ObserveRegister == observationInfo.action)
			{
				m_interestedObservers.push_back(observationInfo.obsId);
			}
			else if(ObserveAction::ObserveUnregister == observationInfo.action)
			{
				m_interestedObservers.erase(std::remove(m_interestedObservers.begin(), 
							m_interestedObservers.end(), 
							observationInfo.obsId),
							m_interestedObservers.end()
							);
			}

			pthread_t threadId;

			std::cout << "\t\trequestFlag : Observer" << std::endl;
			gObservation = 1;
			static int startedThread = 0;

			if(!startedThread)
			{
				pthread_create (&threadId, NULL, ChangeValueRepresentation, (void *)this);
				startedThread = 1;
			}
			ehResult = OC_EH_OK;
		}
	}
	else
	{
		std::cout << "Request Invalid" << std::endl;
	}

	return ehResult;
}

};

void *ChangeValueRepresentation (void *param)
{
	SensorResource *sensorPtr = (SensorResource*) param;
	
	while(1)
	{
		sleep(10);

		if(gObservation)
		{
			sensorPtr->fire_alarm = !sensorPtr->fire_alarm;

			std::cout << "Notifying observers with resource handle: " << sensorPtr->getHandle() << std::endl;

			OCStackResult result = OC_STACK_OK;
			result = OCPlatform::notifyAllObservers(sensorPtr->getHandle());

			if(OC_STACK_NO_OBSERVERS == result)
			{
				std::cout << "No more observers, stopping notifications" << std::endl;
				gObservation = 0;
			}
		}
	}

	return NULL;
}

void *mqttSubscribe(void *)
{
	std::string address = MQTT_ADDRESS;
	std::string clientID = MQTT_CLIENTID2;
	mqtt::async_client client(address, clientID);
	s_action_listener subListener("Subscription");

	s_callback cb(client, subListener);
	client.set_callback(cb);

	mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(20);
	connOpts.set_clean_session(true);

	try
	{
		mqtt::itoken_ptr conntok = client.connect(connOpts);
		conntok->wait_for_completion();
		while(1)
		{
			client.subscribe(TOPIC2, QOS, nullptr, subListener);
		}
	}
	catch (const mqtt::exception& exc)
	{
		std::cerr << "Error: " << exc.what() << std::endl;
	}
}

static FILE* client_open(const char* /*path*/, const char *mode)
{
	return fopen(SVR_DB_FILE_NAME, mode);
}

int main(int argc, char* argv[])
{
	uuid_t id;
	int fd;
	char *tmp = new char[37];
	time_t time_now;

	system("sudo rm -rf ControllerPi1-203.252.146.154-1883");
	system("sudo rm -rf ControllerPi2-203.252.146.154-1883");
	system("sudo ./startmotion");

	// setting uuid
	if(0 < (fd = open("./uuid_log.txt", O_RDONLY)))
	{
		std::cout << "success open" << std::endl;
		read(fd, tmp, UUID_SIZE);
	}
	else
	{
		std::cout << "fail open" << std::endl;
		uuid_generate(id);
		uuid_unparse(id, tmp);
		system("touch uuid_log.txt");
		if(0 < (fd = open("./uuid_log.txt", O_WRONLY)))
		{
			write(fd, tmp, UUID_SIZE);
		}
	}

	controllerID = tmp;
	std::cout << "uuid : '" << controllerID << "'" << std::endl;

	std::cout << "Non-secure resource and notify all observers\n";
	OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink};

	// Create PlatformConfig object
	PlatformConfig cfg {
		OC::ServiceType::InProc,
		OC::ModeType::Server,
		"0.0.0.0",	// all available interfaces
		0,			// randomly available port
		OC::QualityOfService::LowQos,
		&ps
	};

	OCPlatform::Configure(cfg);
	try
	{
		SensorResource mySensor;

		mySensor.createResource();
		std::cout << " -- created resource -- " << std::endl;

		/* mutex */
		//std::mutex blocker;
		//std::condition_variable cv;
		//std::unique_lock<std::mutex> lock(blocker);
		//cv.wait(lock, []{return false;});

		std::cout << " -- waiting resource -- " << std::endl;

		std::string address = MQTT_ADDRESS;
		std::string clientID = MQTT_CLIENTID;
		std::string sensorValue;
		std::string sensorPAYLOAD;
		const char* PAYLOAD;
		mqtt::async_client client(address, clientID);

		// mqtt publish
		callback cb;
		client.set_callback(cb);

		mqtt::connect_options conopts;
		mqtt::message willmsg(" ", 1, true);
		mqtt::will_options will(TOPIC, willmsg);
		conopts.set_will(will);

		mqtt::itoken_ptr conntok = client.connect(conopts);
		conntok->wait_for_completion();

		// mqtt subscribe
		pthread_t threadId;
		static int startedThread = 0;
		if(!startedThread)
		{
			pthread_create(&threadId, NULL, &mqttSubscribe, NULL);
			startedThread = 1;
		}

		while(1)
		{
			sleep(1);

			time(&time_now);

			sensorPAYLOAD = "{\"controller_id\":\"" + controllerID + "\", \"controller_name\":\"" + controllerNAME + "\", \"update_timestamp\":" + std::to_string(time_now)  + ", \"admin_name\":\"" + adminNAME + "\", \"admin_tel\":\"" + adminTEL + "\", \"ip_address\":\"" + controllerIP + "\", \"description\":\"" + description + "\", \"sensor_data\":[";
			int tempCnt = 1;
			vector<Resource *>::iterator vi;
			for(vi=v.begin(); vi!=v.end(); ++vi)
			{
				if(tempCnt == 1)
				{
					tempCnt++;
					continue;
				}
				tempCnt++;
				(*vi)->fire_alarm = isFire;
				sensorValue = "{\"sensor_id\":\"" + (*vi)->m_name + 
					"\", \"sensor_uri\":\"" + (*vi)->m_sensorUri + 
					"\", \"light_state\":" + std::to_string((*vi)->light_state) + 
					", \"light_power\":" + std::to_string((*vi)->light_power) + 
					", \"gas_state\":" + std::to_string((*vi)->gas_state) + 
					", \"gas_efflux\":" + std::to_string((*vi)->gas_efflux) + 
					", \"temp_state\":" + std::to_string((*vi)->temp_state) + 
					", \"temp\":" + std::to_string((*vi)->temper) + 
					", \"humi_state\":" + std::to_string((*vi)->humi_state) +
					", \"humi\":" + std::to_string((*vi)->humi) + 
					", \"flame_state\":" + std::to_string((*vi)->flame_state) + 
					", \"flame_power\":" + std::to_string((*vi)->flame_power) + 
					", \"isFire\":" + std::to_string((*vi)->fire_alarm) + 
					", \"update_timestamp\":" + std::to_string((*vi)->time_stp) +
					", \"person\":" + (*vi)->person +
					"}, ";
				sensorPAYLOAD.append(sensorValue);
			}
			std::cout << sensorPAYLOAD << std::endl;
			if(tempCnt != 2)
			{
				sensorPAYLOAD=sensorPAYLOAD.substr(0, sensorPAYLOAD.length()-2);
			}
			sensorPAYLOAD.append("]}");
			PAYLOAD = sensorPAYLOAD.c_str();

			if(isDelete)
			{
				for(unsigned int i = 0; i < v.size(); i++)
				{
					if((v[i]->m_name).compare(delete_sensor1)==0 && (v[i]->m_sensorUri).compare(delete_sensor2)==0)
				    {
						cout << v[i]->m_sensorUri << " / " << delete_sensor2 << endl;
						v.erase(v.begin()+i);
						isDelete = 0;
					}
				}
			}

			mqtt::message_ptr pubmsg = mqtt::make_message(PAYLOAD);
			pubmsg->set_qos(QOS);
			client.publish(TOPIC, pubmsg)->wait_for_completion(TIMEOUT);
		}		
		std::cout << " * while end * " << std::endl;
	}
	catch(OCException &e)
	{
		std::cout << "OCException in main : " << e.what() << std::endl;
	}

	return 0;
}
