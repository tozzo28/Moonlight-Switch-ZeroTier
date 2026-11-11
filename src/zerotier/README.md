Veja [ZEROTIER-INTEGRATION.md](../../ZEROTIER-INTEGRATION.md) para arquitetura,
origem das dependências, build, validação e roteiro de testes no Switch.

`Embedded.*`: lifecycle e callbacks do Core; `StateStore.*`: estado persistente;
`Transport.cpp`: wrappers de sockets dentro do processo; `net/`: VNet/TCP raw;
`lwip/` e `lwip-port/`: stack e configuração; `include/` e `lib/`: API e Core.
