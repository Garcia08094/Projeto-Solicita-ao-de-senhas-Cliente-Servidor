# Sistema de Senhas Cliente/Servidor

Projeto em C para a disciplina ARA0363 - Programacao de Software Basico em C.

O sistema permite que varios clientes se conectem a um servidor TCP e solicitem senhas sequenciais de atendimento no formato `A001`, `A002`, `A003` e assim por diante.

## Funcionalidades

- Comunicacao cliente/servidor via Socket TCP/IP.
- Suporte a multiplos clientes simultaneos usando threads.
- Geracao de senhas sequenciais controlada pelo servidor.
- Tratamento de desconexoes e comandos invalidos.
- Codigo separado em cliente, servidor e funcoes comuns.
- Compatibilidade com Windows, Linux e macOS.

## Estrutura

```text
sistema_senhas_cliente_servidor/
  README.md
     src/
    client.c
    common.h
    server.c
```

## Como compilar no Linux/macOS

No terminal, dentro da pasta do projeto:

```bash
make
```

Isso gera:

- `build/servidor`
- `build/cliente`

## Como compilar no Windows com GCC/MinGW

No PowerShell ou Prompt de Comando, dentro da pasta do projeto:

```bat
gcc src\server.c -o servidor.exe -lws2_32
gcc src\client.c -o cliente.exe -lws2_32
```

## Como executar

### 1. Iniciar o servidor

Linux/macOS:


./build/servidor 8080


Windows:

```bat
servidor.exe 8080
```

Se a porta nao for informada, o servidor usa a porta `8080`.

### 2. Iniciar um ou mais clientes

Em outros terminais, execute:

Linux/macOS:


./build/cliente 127.0.0.1 8080


Windows:


cliente.exe 127.0.0.1 8080


## Comandos do cliente

Depois de conectado, o cliente pode digitar:

- `1` ou `SOLICITAR`: solicita uma nova senha.
- `2` ou `STATUS`: consulta a ultima senha gerada.
- `3` ou `SAIR`: encerra a conexao.

## Exemplo de uso

Cliente:

```text
Digite uma opcao: 1
Senha gerada: A001
```

Servidor:


Servidor iniciado na porta 8080.
Cliente conectado: 127.0.0.1:53022
Senha A001 gerada para 127.0.0.1:53022
