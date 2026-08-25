#include <stdio.h>
#include <winsock2.h>
#include <stdint.h>

#define PORT 3333

typedef uint8_t bool_t;
#define BOOL_TRUE ((bool_t)1U)
#define BOOL_FALSE ((bool_t)0U)

typedef struct {
	bool_t temperature;
	bool_t wifi;
	bool_t sd;
} anomaly_t;

typedef struct {
	int16_t temperature;
} telemetry_t;

typedef struct {
	anomaly_t anomaly;
	telemetry_t telemetry;
} system_t;

static void deserialize_system_datas(uint8_t* buffer_datas, system_t* system_datas);
static void print_bool_data(char* str, bool_t data);
static void print_datas(system_t* datas);

int main(void)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("Error Winsock : %d\n", WSAGetLastError());
		return 1;
	}

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
		.sin_addr.s_addr = htonl(INADDR_ANY)
	};

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		return 1;
	}

	if(bind(sock, (struct sockaddr*)&addr, (int)sizeof(addr)) < 0)
	{
		return 1;
	}

	while (1)
	{
		int size = (int)sizeof(addr);
		system_t system_datas;
		uint8_t buffer_datas[sizeof(system_t)] = {0};

		int bytes = recvfrom(sock, (char*)buffer_datas, sizeof(system_t), 0, (struct sockaddr*)&addr, &size);
		if (bytes == sizeof(system_t))
		{
			deserialize_system_datas(buffer_datas, &system_datas);
			print_datas(&system_datas);
		}
		else
		{
			printf("Error in reception, bytes: %d\n", bytes);
		}
	}

	closesocket(sock);
	WSACleanup();

	return 0;
}

static void deserialize_system_datas(uint8_t* buffer_datas, system_t* system_datas)
{
	system_datas->anomaly.temperature = buffer_datas[0];
	system_datas->anomaly.wifi = buffer_datas[1];
	system_datas->anomaly.sd = buffer_datas[2];

	system_datas->telemetry.temperature = (int16_t)(buffer_datas[3] << 8) | (uint16_t)(buffer_datas[4]);
}

static void print_bool_data(char* str, bool_t data)
{
	if(data == BOOL_FALSE)
	{
		printf("%s: OK\n", str);
	}
	else
	{
		printf("%s: ANOMALY\n", str);
	}
}

static void print_datas(system_t* datas)
{
	printf("\n\nTemperature: %d\n\n",
		datas->telemetry.temperature
	);
	printf("STATES:\n");
	print_bool_data("Temperature sensor:", datas->anomaly.temperature);
	print_bool_data("Wifi:", datas->anomaly.wifi);
	print_bool_data("SD:", datas->anomaly.sd);
}