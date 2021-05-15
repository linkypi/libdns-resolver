#include "MyEvDns.h"
#include <jni.h>
#include <string>
#include "com_dns_demo_TestDnsResolve.h"

using namespace std;


//jstring to char*
char* jstringTostring(JNIEnv* env, jstring jstr)
{
	char* rtn = NULL;
	jclass clsstring = env->FindClass("java/lang/String");
	jstring strencode = env->NewStringUTF("utf-8");
	jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr = (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
	jsize alen = env->GetArrayLength(barr);
	jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
	if (alen > 0)
	{
		rtn = (char*)malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	env->ReleaseByteArrayElements(barr, ba, 0);
	return rtn;
}

typedef struct
{
	jobject cbInstance;
	jmethodID cbMethod;
} Jni_Callback;

// 定义承载 Java-Listener 和 Java-Callback 实例和 methodID 的结构体实例
static Jni_Callback* jni_callback;

static JavaVM* jvm;
static jclass jniCallbackClass;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	// 把 JavaVM 存下来，在回调函数中通过它来得到 JNIEnv
	jvm = vm;
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8) != JNI_OK) {
		return JNI_ERR;
	}

	// 把回调函数的 jclass 通过 GlobalRef 的方式存下来，因为 jclass (是 Local Reference)不能跨线程
	jniCallbackClass = (jclass)env->NewGlobalRef(env->FindClass("com/dns/demo/DnsCallback"));
	return JNI_VERSION_1_8;
}


JNIEXPORT void JNICALL Java_com_dns_demo_TestDnsResolve_resolveDns
(JNIEnv* env, jclass object, jstring hostName, jobject callback) {
	if (!callback)
		return;

	// 获得回调接口 函数call 的 methodID.
	//jclass clazz_listener = env->GetObjectClass(callback);
	jmethodID callbackMethod = env->GetMethodID(jniCallbackClass, "call", "(Lcom/dns/demo/CallbackResponse;)V");

	// 这里创建 jni_callback_t 的实例，创建 Listener 和 Callback 的全局引用并赋值.
	jni_callback = (Jni_Callback*)malloc(sizeof(Jni_Callback));
	memset(jni_callback, 0, sizeof(Jni_Callback));
	jni_callback->cbInstance = env->NewGlobalRef(callback);
	jni_callback->cbMethod = callbackMethod;
	//jni_callback->z_greet_callback = (jclass)env->NewGlobalRef(clazz_greet_callback);
	//jni_callback->m_greet_callback_init = method_greet_callback_init;

	// 销毁局部引用
	//env->DeleteLocalRef(clazz_listener);
	//env->DeleteLocalRef(clazz_greet_callback);
	//clazz_listener = NULL;
	//clazz_greet_callback = NULL;
	char* host = jstringTostring(env, hostName);
	resolve_dns(host);

	free(host);
	free(jni_callback);
	host = NULL;
	jni_callback = NULL;
}

/**
*  callback for jni
*/
jobject buildIPArray(JNIEnv* env, int count, char** ips) {
	/*jclass class_hashmap = env->FindClass("java/util/HashMap");
	jmethodID hashmap_construct_method = env->GetMethodID(class_hashmap, "<init>", "()V");
	printf("get hashmap_construct_method success \n");
	jobject obj_hashmap = env->NewObject(class_hashmap, hashmap_construct_method);
	jmethodID hashmap_put_method = env->GetMethodID(class_hashmap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	printf("get hashmap_put_method success \n");*/
	// new一个ArrayList对象
	jclass arr_class = env->FindClass("java/util/ArrayList");
	jmethodID construct_method = env->GetMethodID(arr_class, "<init>", "()V");
	jobject arr = env->NewObject(arr_class, construct_method);
	jmethodID add_method = env->GetMethodID(arr_class, "add", "(Ljava/lang/Object;)Z");
	printf("inti array success, arr ponter: %d \n", arr);

	for (int index = 0; index < count; index++)
	{
		printf("build arraylist: %s \n", ips[index]);
		jstring element = env->NewStringUTF(ips[index]);
		env->CallBooleanMethod(arr, add_method, element);
	}
	
	return arr;
}

void callbackFunc(int errCode, int count, int ttl, char* originHost, char** ips)
{
	// JNI_OnLoad 时保存 jvm，这里使用 jvm->AttachCurrentThread 获取 JNIEnv，暂不做详细介绍.
	JNIEnv* env;
	jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);

	// 获得自定义 Callback类 GreetCallback 构造函数的 methodID.
	jclass clazz_callback_res = env->FindClass("com/dns/demo/CallbackResponse");
	// int code, int count , int ttl, String origin, Map<Integer, List<String>> mapper
	jmethodID method_callback_res_init = env->GetMethodID(clazz_callback_res, "<init>", "(IIILjava/lang/String;Ljava/util/List;)V");
	printf("get CallbackResponse class success \n");

	jobject ipArr = NULL;
	if (errCode == 0) {
		ipArr = buildIPArray(env, count, ips);
	}
	printf("build arraylist success \n");
	jstring jhost = env->NewStringUTF(originHost);
	jobject response = env->NewObject(clazz_callback_res, method_callback_res_init, errCode, count, ttl, jhost, ipArr);
	printf("create response success \n");
	env->CallVoidMethod(jni_callback->cbInstance, jni_callback->cbMethod, response);
	printf("invoke callback method success \n");

	// 销毁全局引用 --- 如果有多处或多次回调，自行判断销毁时机.
	//env->DeleteGlobalRef(jni_callback->cbInstance);
	//env->DeleteLocalRef(ipArr);  //删除局部引用
	//env->DeleteGlobalRef(jni_callback->cbMethod);
	jni_callback->cbInstance = NULL;
	jni_callback->cbMethod = NULL;
	//if (ipArr) {
	//	free(ipArr);
	//}
	free(jni_callback);
	jni_callback = NULL;

	// 这个必须和 AttachCurrentThread 方法成对出现
	jvm->DetachCurrentThread();
}


static char* numberToAddr(u32 address)
{
	// 注意此处不可直接写成 static char* buf; 该方式会导致返
	// 回到Java中的数组元素都是同一个, 即指针指向了同一个位置
	// 故推荐 char* buf = (char*)malloc(16);
	char* buf = (char*)malloc(16);
	u32 a = ntohl(address);
	sprintf(buf, "%d.%d.%d.%d",
		(int)(u8)((a >> 24) & 0xff),
		(int)(u8)((a >> 16) & 0xff),
		(int)(u8)((a >> 8) & 0xff),
		(int)(u8)((a) & 0xff));
	return buf;
}

void free2DArray(void** arr, int count) {
	for (int i = 0; i < count; i++) {
		free(arr[i]);
	}
	delete[]arr;
	arr = NULL;
	printf("free memory success\n");
}


DnsCallback callback = NULL;

static void resolve_callback(int errCode, char type, int count, int ttl, void* addrs, void* originHost) {
	char* host = (char*)originHost;
	char** result = new char* [count];

	if (errCode == DNS_ERR_NONE) {
		for (int i = 0; i < count; ++i) {
			if (type == DNS_IPv4_A) {
				char* ip = numberToAddr(((u32*)addrs)[i]);
				result[i] = ip;
				printf("%s: %s , index: %d, result[i]: %s \n", host, ip, i, result[i]);
			}
			else if (type == DNS_PTR) {
				char* dnsPtr = ((char**)addrs)[i];
				result[i] = dnsPtr;
				printf("%s: %s\n", host, dnsPtr);
			}
		}
	}

	// jni callback
	if (jniCallbackClass) {
		callbackFunc(errCode, count, ttl, host, result);
	}
	// jna callback
	if (callback != NULL) {
		try {
			callback(errCode, type, count, ttl, host, result);
		}
		catch (...) {
		}

		free2DArray((void**)result, count);
	}
}

void resolve_dns(char* host_name) {
	struct event_base* event_base = NULL;
	struct evdns_base* evdns_base = NULL;

#ifdef _WIN32
	{
		WSADATA WSAData;
		WSAStartup(0x101, &WSAData);
	}
#endif

	event_base = event_base_new();
	evdns_base = evdns_base_new(event_base, EVDNS_BASE_DISABLE_WHEN_INACTIVE);

	int res;
#ifdef _WIN32
	res = evdns_base_config_windows_nameservers(evdns_base);
#endif
	/*	if (o.ns)
			res = evdns_base_nameserver_ip_add(evdns_base, o.ns);
		else
			res = evdns_base_resolv_conf_parse(evdns_base,
				DNS_OPTION_NAMESERVERS, o.resolv_conf);*/
	evdns_base_resolve_ipv4(evdns_base, host_name, 0, resolve_callback, host_name);
	event_base_dispatch(event_base);
	evdns_base_free(evdns_base, 1);
	event_base_free(event_base);
}

void resolve(char* host, DnsCallback func)
{
	callback = func;
	resolve_dns(host);
}
