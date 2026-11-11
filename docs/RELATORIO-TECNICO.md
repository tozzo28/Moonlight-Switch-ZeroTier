# Moonlight com ZeroTier — recompilação limpa para teste

O aplicativo, ZeroTier Core e curl foram recompilados no Ubuntu via WSL 1 com o devkitA64 16.1.0 transferido do ambiente original. Não foi feita edição direta deste NRO.

Padrões: ZeroTier desativado, rede e peer vazios. O usuário informa sua própria rede e PC. O tamanho dos pacotes consulta o peer configurado no transporte, em vez de um IP pessoal fixo.

Não inclui identidade ZeroTier, chaves/certificados de pareamento, hosts, perfis, logs ou capas do usuário. O novo dispositivo e o pareamento são criados no primeiro uso.

## Instalação e teste

Copie a pasta switch para o cartão e execute Moonlight pelo hbmenu em contexto com acesso completo à RAM. Para validar o primeiro uso, utilize uma instalação de teste sem os dados anteriores do Moonlight. Copiar sobre um cartão já configurado não apaga nem neutraliza configurações, identidades ou certificados que já estiverem nele.

Configure sua rede ZeroTier e o IP virtual do PC, habilite ZeroTier e feche/reabra o aplicativo. Autorize o novo dispositivo na sua rede, adicione o PC com Sunshine, faça o pareamento e teste primeiro 720p60 com bitrate moderado. Verifique imagem, controles, encerramento e reabertura. Não copie identidades ou chaves de outro usuário.

## Verificações realizadas

- NRO0 e ASET válidos, segmentos e assets dentro dos limites; ELF AArch64/PIE.
- Rede virtual: 88 verificações, zero falhas.
- Opções: padrões vazios/desativados e validação de entradas.
- Transporte UDP/TCP, prontidão, timeouts/erros, rede nativa e queda do link; dois conjuntos de endereços sintéticos.
- ENet: conexão e controle confiável bidirecional nos dois cenários sintéticos.
- Gestos da interface, cancelamento HTTP, retomada/launch e migração de perfis: testes aprovados.
- Ausência dos identificadores pessoais conhecidos em UTF-8 e UTF-16, das chaves e da identidade originais; ausência dos IDs numéricos conhecidos de rede e nó. Veja VERIFICACAO.json.

Não foi executado no Switch após a recompilação. Testes sintéticos não comprovam streaming real ou desempenho. A verificação documenta os dados conhecidos e não constitui promessa de ausência absoluta de informação desconhecida.

## Fontes e dependências

Acompanhe esta entrega com moonlight-fontes-limpas.zip. Ela contém as fontes revisadas, scripts e avisos de licença. SDK devkitPro/portlibs são pré-requisitos externos.

As quatro bibliotecas FFmpeg foram preservadas como dependências públicas: seus blobs são idênticos aos do Moonlight-Switch upstream no commit 4d093fe7c5f6a3b7d7c431ca4ba4903a391f008e. Não foram recompiladas nesta sessão. Seus hashes e URLs estão em modern/lib/switch/UPSTREAM-VERIFICADO.json no pacote de fontes. O Core e o curl foram recompilados.

Créditos: XITRIX/Moonlight-Switch; ZeroTier; sys-GRID0-tcp/VNet; lwIP; curl; FFmpeg/Averne; e demais componentes vendorizados. Licenças principais foram preservadas na pasta licenses e na árvore de fontes.

## Antes de publicar

A duvida principal de compatibilidade foi esclarecida: o VNet/port utilizado e MPL-2.0. A atribuicao GPL-2.0 vinha da pasta externa e nao dos fontes usados. Veja LICENCAS-REVISAO.md para as evidencias, condicoes de distribuicao e limites da revisao. Preserve avisos, creditos e fontes ao distribuir. Nada foi publicado no GitHub.
