# Dropbox

Trabalho final para a cadeira de Sistemas Operacionais 2, do professor Alberto Egon Schaeffer Filho.
O objetivo é implementar um clone do Dropbox, que faz sincronização dos arquivos de usuários conectados em vários dispositivos, numa arquitetura cliente-servidor.

# Funcionamento

## Clientes

Os clientes devem se conectar no ip do servidor eleito, na porta `12345`.

## Servidores

Os servidores precisam de um arquivo de configuração para funcionar. Ele deve estar no `current working dir`, possuir o nome `config.toml`, onde deve haver uma variável `servers` que é um vetor de objetos do tipo `ip: string, port: number`. O arquivo deve ser formatado conforme a especificação TOML.

Exemplo de arquivo de configuração:

```toml
servers = [
    { ip = "123.321.123.321", port = 123 },
    { ip = "127.0.0.1", port = 456 },
    { ip = "34.79.255.6", port = 789 },
]
```

Para inicializar, basta iniciar o executável do servidor com um índice que corresponde ao vetor. Desta forma, tomando o exemplo acima, `./server 1` inicia o servidor com no ip `127.0.0.1`, na porta `456`. Note que esse índice também será a id do servidor, que é contabilizada no processo de eleição. 