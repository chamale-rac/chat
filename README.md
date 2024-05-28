# 🗨️ Sistema de Chat

## Creación de un Sistema de Chat para el Curso de Sistemas Operativos

### Requisitos

Para crear un sistema de chat funcional para el curso de Sistemas Operativos, se deben seguir varios pasos, incluyendo la instalación de las herramientas necesarias, la compilación del código y la comprensión de los protocolos de comunicación subyacentes.

### Instalación de `protoc`

Para instalar `protoc`, el compilador de buffers de protocolo, siga las instrucciones proporcionadas en la [guía de instalación de gRPC protoc](https://grpc.io/docs/protoc-installation/).

Alternativamente, puede utilizar los siguientes comandos para la instalación en un sistema basado en Debian:

```bash
sudo apt update && sudo apt install -y protobuf-compiler libprotobuf-dev
```

## Compilación

Después de configurar las herramientas necesarias, es necesario compilar los archivos protobuf y luego compilar las aplicaciones del cliente y del servidor.

### Generación de Archivos Protobuf

Primero, genere los archivos protobuf necesarios utilizando el siguiente comando:

```bash
protoc --proto_path=./utils --cpp_out=./utils ./utils/chat.proto
```

### Compilación del Cliente y del Servidor

A continuación, compile las aplicaciones del cliente y del servidor con los siguientes comandos:

```bash
g++ -o ./executables/client client.cpp ./utils/chat.pb.cc ./utils/message.cpp ./utils/constants.h -lprotobuf
g++ -o ./executables/server server.cpp ./utils/chat.pb.cc ./utils/message.cpp ./utils/constants.h -lpthread -lprotobuf
```

### Ejecución del Servidor y del Cliente

Finalmente, ejecute las aplicaciones del servidor y del cliente de la siguiente manera:

```bash
./executables/server 
```
> Uso: `./executables/server port server_name`

```bash
./executables/client
```
> Uso: `./executables/client server_IP server_port username`

## Peculiaridades de la Implementación

1. **Manejo de Mensajes Broadcast**:
   - Cuando se envía un mensaje broadcast, este no se envía al cliente que lo envió originalmente. Esto asegura que el remitente no reciba su propio mensaje de vuelta, evitando redundancias y posibles confusiones.

2. **Variables Configurables**:
   - La implementación incluye varias variables configurables que pueden ajustarse globalmente para modificar el comportamiento del sistema. Estas variables son:
     ```cpp
     // Bandera booleana para manejar IP ÚNICA
     constexpr bool HANDLE_UNIQUE_IP = false;

     // Bandera entera para manejar el estado offline automático en segundos
     constexpr int AUTO_OFFLINE_SECONDS = 10;

     // Indica el tamaño estático del búfer
     constexpr size_t BUFFER_SIZE = 64 * 1024; // Esto es 64 KB

     // Indica si se usa el modo verbose
     constexpr bool VERBOSE = false;
     ```
   - Estas variables permiten ajustar aspectos como la gestión de IPs únicas, el tiempo de espera para considerar a un cliente como desconectado, el tamaño del búfer para la comunicación y el nivel de detalle en los registros de operación.

3. **Manejo de Clientes con Hilos Dedicados**:
   - En el servidor, cada cliente se maneja con un hilo (thread) dedicado. Esta arquitectura multihilo permite que el servidor gestione múltiples conexiones de clientes simultáneamente, mejorando la capacidad de respuesta y la escalabilidad del sistema.

4. **Manejo de Desconexiones y Terminaciones Abruptas**:
   - La implementación está diseñada para manejar de manera robusta las desconexiones y terminaciones abruptas, tanto para el cliente como para el servidor. Esto incluye la gestión de errores de red y la terminación deliberada de procesos.
   - Se han implementado mecanismos para detectar y manejar estas situaciones, asegurando que el sistema se recupere adecuadamente o termine de manera ordenada, minimizando el impacto en la comunicación y en la integridad de los datos.

## Comandos Disponibles

La aplicación de chat soporta los siguientes comandos:

```plaintext
send <message>
sendto <recipient> <message>
status <status>
list
info <username>
help
stream
exit
```

## Tabla de Librerías

| Biblioteca | Descripción |
| --- | --- |
| `<iostream>` | Biblioteca para operaciones de entrada/salida estándar. |
| `<sys/socket.h>` | Biblioteca para operaciones con sockets. |
| `<arpa/inet.h>` | Biblioteca para operaciones de internet (como convertir direcciones IP). |
| `<unistd.h>` | Biblioteca para operaciones de POSIX (como cerrar sockets). |
| `<cstring>` | Biblioteca para manipulación de cadenas C. |
| `<string>` | Biblioteca para manipulación de cadenas C++. |
| `<thread>` | Biblioteca para manejo de hilos. |
| `<vector>` | Biblioteca para manejar vectores (arrays dinámicos). |
| `<atomic>` | Biblioteca para operaciones atómicas (para variables compartidas entre hilos). |
| `<deque>` | Biblioteca para manejar colas dobles (double-ended queues). |
| `<mutex>` | Biblioteca para manejo de mutex (para sincronización de hilos). |
| `<condition_variable>` | Biblioteca para manejo de variables de condición (para sincronización de hilos). |
| `<sstream>` | Biblioteca para manipulación de cadenas con stream. |

## IDE y Lenguajes

El desarrollo de este proyecto se realizó utilizando las siguientes herramientas y tecnologías:

| Categoría | Herramienta/Descripción |
| --- | --- |
| IDE | <img src="https://img.shields.io/badge/Visual_Studio_Code-007ACC.svg?style=for-the-badge&logo=Visual-Studio-Code&logoColor=white"/> |
| Lenguaje | <img src="https://img.shields.io/badge/C++-00599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white"/> |

---

## Documento con Investigaciones Complementarias

Documento PDF: [Investigaciones Complementarias](./Investigaciones.pdf)