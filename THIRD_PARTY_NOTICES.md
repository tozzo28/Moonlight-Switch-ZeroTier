# Componentes, licenças e proveniência

Este é um fork modificado, sem vínculo oficial com os projetos de origem.
Os avisos de autoria e os textos originais de licença permanecem nas árvores vendorizadas.

| Componente | Referência de licença/proveniência nesta árvore |
| --- | --- |
| Moonlight-Switch / XITRIX | `LICENSE`, `modern/LICENSE`; base da interface no commit `4d093fe7c5f6a3b7d7c431ca4ba4903a391f008e` |
| Moonlight Common C e ENet | `modern/extern/moonlight-common-c/LICENSE.txt`, `modern/extern/moonlight-common-c/enet/LICENSE` |
| VNet/TcpStack e compatibilidade do port | MPL-2.0; `src/zerotier/net/LICENSE`, `zerotier-core/compat/LICENSE`; `Jul879n/sys-GRID0-tcp` no commit `8a865f8736b466e1fd23d33088653bd91c5fc78f` |
| ZeroTier Core | `zerotier-core/ZeroTierOne/LICENSE.txt` e `LICENSE-MPL.txt`; fontes `node/` sob MPL-2.0, commit `11186cc77de5499e26e589570d2be9c097319e9f` |
| lwIP | `src/zerotier/lwip/COPYING` e avisos nos arquivos |
| curl 7.69.1 | `curl/source/COPYING`; fonte upstream arquivado e verificado no script |
| FFmpeg com NVDEC/NVTEGRA | GPL-2.0 ou posterior conforme configuração dos binários; `third_party/ffmpeg-source/`; referência `e932809b8fc4c2a434cc2f2360e0e12de901b268` de `averne/FFmpeg` |
| Borealis / Yoga / fmt / demais bibliotecas da interface | Licenças e avisos nas respectivas pastas de `modern/extern/borealis` |
| Nanogui e dependências do frontend legado | Avisos em `third_party/nanogui` e seus componentes |

As quatro bibliotecas FFmpeg em `modern/lib/switch/` são idênticas aos blobs públicos do
Moonlight-Switch no commit acima. Veja `UPSTREAM-VERIFICADO.json`. Elas não foram recompiladas
nesta entrega. Os fontes FFmpeg incluídos foram identificados pela versão embutida nos binários;
isso não é uma reprodução byte a byte dessas bibliotecas.

Os arquivos MPL usados na combinação com o aplicativo GPL-3.0 são disponibilizados também
sob GPL-3.0 nos termos da seção 3.3 da MPL-2.0, preservando os avisos e a disponibilidade sob MPL.
Essa opção não altera os termos de outros componentes. Veja `docs/LICENCAS-REVISAO.md`.

Os diretórios históricos, de ferramentas e `nonfree/` no checkout ZeroTier conservam seus
próprios avisos. O build deste aplicativo usa o Core `node/` e os componentes definidos em
`zerotier-core/zt-core.mk`, não o componente separado libzt nem o controlador nonfree.

Ao distribuir, mantenha os fontes, scripts e avisos correspondentes às bibliotecas usadas.
O SDK e suas portlibs são dependências externas com termos próprios. A revisão registrada
tem o escopo descrito no relatório; não substitui uma auditoria jurídica integral.
