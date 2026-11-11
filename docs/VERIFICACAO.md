# Resultado atualizado

Compilação concluída com sucesso no Ubuntu 26.04 via WSL 1, CMake 4.2.3 e devkitA64 16.1.0. O erro anterior de fsync no Windows não se reproduziu no teste Linux: migração de perfis aprovada.

ZeroTier Core, curl, zstd, bibliotecas da interface e Moonlight foram recompilados. As quatro bibliotecas FFmpeg são binários públicos verificados contra os blobs upstream, não recompilados. SDK e portlibs foram restaurados do ambiente original, sem alteração dos projetos de origem.

Padrões sem rede/peer e ZeroTier desativado. Corrigida a inicialização desativada com campos vazios e removida a dependência de um IP pessoal para o tamanho dos pacotes. CMake corrigido para preservar as opções da plataforma e funcionar sem o cache antigo.

Testes aprovados: opções; 88 verificações VNet; transporte nativo e virtual, TCP/UDP e queda de link com dois conjuntos de IPs sintéticos; ENet confiável bidirecional nos dois conjuntos; gestos; cancelamento HTTP; resume/launch; migração de perfis.

Varredura dos identificadores pessoais conhecidos e das chaves/identidade originais: nenhuma ocorrência encontrada no novo NRO. Header NRO0, ASET, limites dos assets e arquitetura AArch64/PIE conferidos. O caminho local da compilação não consta no NRO.

Atualizacao de 10/11/2026: usuario informou funcionamento no Switch. A atribuicao de licenca do VNet foi corrigida para MPL-2.0; veja LICENCAS-REVISAO.md. Fontes FFmpeg da revisao identificada foram incluidos. Nao se afirma garantia absoluta nem auditoria juridica integral.

NRO SHA-256: c76239a3883a1e9a35c5a9672ea4336308d597579ecdc44d30e2f6dc420c22d7
