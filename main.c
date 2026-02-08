#include <Windows.h>
#include <stdio.h>
#include <time.h>

#define MAX_QUEUE 200
CRITICAL_SECTION criticalSection;
CONDITION_VARIABLE conditionalVariable;

char ThreadQueue[MAX_QUEUE];
int head = 0;
int tail = 0;
int count = 0;

BOOL doneProducing = FALSE;

void push(char c) {
	ThreadQueue[tail] = c;
	tail = (tail + 1) % MAX_QUEUE;
	count++;
}

char pop() {
	char c = ThreadQueue[head];
	head = (head + 1) % MAX_QUEUE;
	count--;

	return c;
}

DWORD consumerThread(LPVOID var) {
	srand(time(NULL));

	while (1) {
		__try {
			Sleep(rand() % 150);

			EnterCriticalSection(&criticalSection);

			while (count == 0 && !doneProducing) {
				SleepConditionVariableCS(&conditionalVariable, &criticalSection, INFINITE);
			}

			if (count == 0 && doneProducing) {
				break;
			}

			char c = pop();

			LeaveCriticalSection(&criticalSection);
			printf("Consumed: %c\n", c);
			
		}
		__finally {
			if (TryEnterCriticalSection(&criticalSection)) {
				LeaveCriticalSection(&criticalSection);
			}
		}
	}

	printf("Consumer finished.\n");
	return 0;
}



DWORD producerThread(LPVOID var){
	srand(time(NULL));

	if (var == NULL) return;
	char* string = (char*)var;

	for (int i = 0; string[i] != '\0'; i++) {
		
		Sleep(rand() % 100);

		__try {
			EnterCriticalSection(&criticalSection);
			push(string[i]);
			printf("Produced: %c\n", string[i]);
		}

		__finally {
			LeaveCriticalSection(&criticalSection);
		}

		WakeConditionVariable(&conditionalVariable);
	}

	__try {
		EnterCriticalSection(&criticalSection);
		doneProducing = TRUE;
		printf("Done producing\n");
	}
	__finally {
		LeaveCriticalSection(&criticalSection);
	}

	return 0;
}

int main() {
	const char* string = "HELLO WORLD";

	InitializeCriticalSection(&criticalSection);
	InitializeConditionVariable(&conditionalVariable);

	HANDLE HproducerThread = CreateThread(NULL, 0, producerThread, (LPVOID)string, 0, NULL);
	HANDLE HconsumerThread = CreateThread(NULL, NULL, consumerThread, NULL, 0, NULL);

	WaitForSingleObject(HproducerThread, INFINITE);
	WaitForSingleObject(HconsumerThread, INFINITE);

	CloseHandle(HproducerThread);
	CloseHandle(HconsumerThread);

	DeleteCriticalSection(&criticalSection);

	return 0;
}
