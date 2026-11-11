# Revisão de licenças — 10/11/2026

## Conclusão da dúvida principal

A suspeita anterior de conflito entre GPL-2.0 e GPL-3.0 no VNet não se confirmou. Ela resultou de um arquivo GPL-2.0 copiado da pasta externa zerotier-nx-moonlight para src/zerotier/net/LICENSE. Os arquivos de rede usados vêm do submódulo Jul879n/sys-GRID0-tcp, commit 8a865f8736b466e1fd23d33088653bd91c5fc78f, cujo LICENSE.txt é MPL-2.0. A origem foi verificada nos arquivos e não apenas no reconhecimento automático de licença pelo GitHub.

Cinco arquivos VNet são byte a byte idênticos à origem: lifetime.hpp, net.hpp, tcp_stack.cpp, tcp_stack.hpp e vnet.cpp. vnet.hpp é uma adaptação desses fontes: comentários e limites de sockets/fila foram alterados. lwip_unity.c deriva do arquivo homônimo, com caminhos de includes ajustados. Todos mantêm a proveniência MPL do port; as fontes lwIP incluídas conservam seus próprios avisos BSD. Os arquivos compat/ usados no Core também foram comparados com os blobs upstream, sem divergências.

A licença errada da cópia exportada foi substituída pelo LICENSE.txt MPL-2.0 da origem, verificado byte a byte com a transferência. Isso corrige a atribuição; não é uma troca arbitrária de licença de código GPL-2.0. Projetos originais preservados.

## Componentes examinados

| Componente | Termos identificados | Observação |
| --- | --- | --- |
| Moonlight-Switch e integração do aplicativo | GPL-3.0 | LICENSE original preservado |
| VNet/TcpStack e compatibilidade importados de sys-GRID0-tcp | MPL-2.0 | Origem fixada no commit acima; modificações documentadas |
| ZeroTier Core deste checkout | MPL-2.0 | Avisos de node/ e LICENSE-MPL.txt; componentes ext/ conservam seus termos; não se usa libzt nesta integração |
| lwIP | BSD | COPYING e avisos dos arquivos preservados |
| curl 7.69.1 | licença curl | COPYING e arquivo-fonte conferido pelo script de compilação |
| FFmpeg estático usado no NRO | GPL versão 2 ou posterior, conforme declaração embutida | --enable-gpl; não encontrado --enable-nonfree na configuração registrada |

A Mozilla permite combinar MPL-2.0 com GPL como obra maior, nas condições da seção 3.3 e da FAQ Q14. Não foi encontrado aviso aplicado de incompatibilidade com licenças secundárias nos fontes MPL usados. A simples presença do texto Exhibit B no documento completo da licença não constitui esse aviso (FAQ Q25).

O código MPL coberto que integra esta combinação é também disponibilizado sob GPL-3.0 nos termos da seção 3.3. Permanecem preservados os avisos MPL e a disponibilidade desses fontes sob MPL. Essa declaração não muda as licenças de arquivos BSD, MIT, curl ou outros componentes.

A opção GPL-2.0 ou posterior do FFmpeg permite escolher GPL-3.0 para esta combinação, diferentemente de GPL-2.0-only. Não foi identificado o conflito específico inicialmente apontado nos componentes examinados.

## Fontes FFmpeg e limites da verificação

As quatro bibliotecas FFmpeg continuam idênticas aos blobs oficiais do Moonlight-Switch no commit 4d093fe7c5f6a3b7d7c431ca4ba4903a391f008e. A string de versão embutida é N-117227-ge932809b8f. O commit completo correspondente foi resolvido no repositório averne/FFmpeg: e932809b8fc4c2a434cc2f2360e0e12de901b268.

Incluídos em third_party/ffmpeg-source/: arquivo-fonte desse commit, licenças, hash e configuração registrada nos binários. Os três headers de versão examinados no arquivo-fonte coincidem byte a byte com os headers fornecidos com as bibliotecas. Isso estabelece uma referência de proveniência, não uma reprodução byte a byte das bibliotecas FFmpeg nem prova de ausência de patches não declarados. Não foi feita auditoria jurídica integral de todos os arquivos e bibliotecas transitivas do SDK.

## Para distribuir

Distribuir o binário com as fontes revisadas e scripts, preservar créditos e licenças, identificar que é uma versão modificada e manter disponíveis os fontes MPL. Conservar também a proveniência e as fontes das dependências; somente publicar um NRO e o texto GPL não substitui as obrigações de disponibilização dos fontes correspondentes. Não apresentar este trabalho como versão oficial da ZeroTier ou XITRIX.

Nenhuma mensagem foi enviada aos autores e nada foi publicado. A dúvida principal foi resolvida por evidência pública; não foi necessário solicitar uma autorização especial para ela. Esta revisão é técnica/documental e tem o escopo descrito acima, sem garantia jurídica absoluta de toda a distribuição.

## Fontes primárias

- Licença do port no commit utilizado: https://github.com/Jul879n/sys-GRID0-tcp/blob/8a865f8736b466e1fd23d33088653bd91c5fc78f/LICENSE.txt
- Arquivos de rede: https://github.com/Jul879n/sys-GRID0-tcp/tree/8a865f8736b466e1fd23d33088653bd91c5fc78f/source/net
- FAQ Mozilla Q14 e Q25: https://www.mozilla.org/en-US/MPL/2.0/FAQ/
- Termos MPL-2.0, seção 3.3: https://www.mozilla.org/en-US/MPL/2.0/
- GNU sobre GPLv2 e GPLv3: https://www.gnu.org/licenses/gpl-faq.en.html#v2v3Compatibility
- FFmpeg na revisão identificada: https://github.com/averne/FFmpeg/tree/e932809b8fc4c2a434cc2f2360e0e12de901b268
