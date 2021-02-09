#include "defines.h"
#include "GUI/ComPort.h"
#include "GUI/rs232.h"
#include <cstring>
#include <ctime>


static int openedPort = -1;


bool ComPort::Open() //-V2506
{
    char mode[] = { '8', 'N', '1', '\0' };

    uint8 buffer[4096];

    uint8 message[] = "*idn?\x0d";

    for (int i = 0; i < 30; i++)
    {
        if (RS232_OpenComport(i, 115200, mode, 0) == 0)
        {
            RS232_SendBuf(i, message, 6);

            int n = RS232_PollComport(i, buffer, 4095);

            if (n > 0)
            {
                openedPort = i;
                return true;
            }

            RS232_CloseComport(i);
        }
    }

    return false;
}

void ComPort::Close()
{
    if (openedPort != -1)
    {
        RS232_CloseComport(openedPort);
    }
}


bool ComPort::IsOpened()
{
    return openedPort != -1;
}


void ComPort::Send(pchar buffer)
{
    if (IsOpened())
    {
        char *p = const_cast<char *>(buffer);

        RS232_SendBuf(openedPort, reinterpret_cast<uint8 *>(p), static_cast<int>(std::strlen(buffer)));
    }
}

int ComPort::Receive(char *buffer, int size, int timeWait) //-V2506
{
    if (IsOpened())
    {
        int received = 0;

        clock_t timeStart = clock();

        while (received < size && clock() < timeStart + timeWait)
        {
            int newBytes = RS232_PollComport(openedPort, reinterpret_cast<unsigned char *>(buffer + received), size - received);
            received += newBytes;
        }

        return received;
    }

    return 0;
}
