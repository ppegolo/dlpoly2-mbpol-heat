real(8), private, parameter :: poly3b_kXX(3) = (/1.4d0, 1.4d0, 1.4d0/)
real(8), private, parameter :: poly3b_coeffs(131) = (/&
& -1.30279495504321d-01,  1.48432067974671d-01, -8.76139585441887d-03, &
&  2.88224095837835d-01, -2.39315393892762d-01, -4.39270124222569d-01, &
&  7.91690857765304d-01,  7.54461730916801d-02, -2.62902151565512d-01, &
& -2.95579432264093d-01, -4.36135431806826d-01,  5.13262236584400d-01, &
& -1.06129256881025d+00,  3.93874119483000d-02,  4.04953402256675d-02, &
& -1.12974344316509d-01, -1.45244828696249d-01,  9.99702780475335d-01, &
&  2.63949264289990d-01,  4.04111930593417d-03, -3.21032961909452d-02, &
&  2.66166392799118d-02, -4.56944489324064d-02, -6.52823886434158d-01, &
& -2.80539504373741d-01,  6.14805229348420d-01, -3.95416579753503d-02, &
&  4.58465671490562d-01, -2.54177579306433d-01, -7.50800960774062d-02, &
&  7.31403207856185d-03,  2.21184567329681d-02,  9.85047647408115d-01, &
& -8.63101088092414d-02, -7.21143447848052d-02,  6.63790567588182d-02, &
&  1.84281713542708d-01, -2.75712024566074d-01, -6.70933572702353d-02, &
&  3.97649403630475d-01,  4.89786063838299d-02,  2.89519006791704d-01, &
& -5.83406665917814d-01,  8.29201570570911d-02,  4.11968800831276d-02, &
& -4.52528915907042d-01,  3.69925070798241d-01, -3.28629537880351d-02, &
& -3.70624035024789d-02, -7.33850958957510d-03, -3.73000831246612d-01, &
&  1.40407408686832d+00,  1.71056298832191d-01, -1.46805027463257d-01, &
&  1.08580976432353d-02,  5.44977195355208d-02, -1.42013171920552d-01, &
&  4.09180799132755d-02, -1.49591793490032d-01, -1.77897053151202d+00, &
&  1.14159832074238d-01, -1.73510317087674d-02, -4.81030323671343d-01, &
& -8.14497016581299d-01, -1.99536253454914d-02, -4.64801129809196d-01, &
& -1.65775012798871d-02, -1.63540470829871d-01,  8.69175750453748d-01, &
&  4.14415400072606d-01,  2.86730541161423d+00,  1.27214516657755d-01, &
& -9.84247332751964d-02, -2.11151118108067d-02,  2.42786786097366d-01, &
& -1.28211364945088d-03,  6.37329408809247d-01,  1.75592630165630d-01, &
& -5.61965512032934d-01, -9.56100912226149d-03,  2.33565179020321d-01, &
&  1.81213157852835d-01,  9.45970600800236d-01, -1.21427801278414d-01, &
&  1.35242147249469d+00, -3.40817389429147d-01, -6.30436414675946d-01, &
&  1.14409622458697d-01, -1.54224499382338d+00,  8.06310084627552d-02, &
&  2.03055829963337d-02, -1.03270273248790d-01,  1.14242543859712d-01, &
&  6.21817354400233d-02, -1.25091475087036d+00,  1.48270624324779d-02, &
&  2.25239910330385d-01,  3.06347063085572d-01,  8.30064323430667d-01, &
& -5.15372812549678d-01, -3.51188468444243d-01,  1.11247438308725d-01, &
& -5.82176045103836d-01, -1.16468863310126d+00,  1.57665856314853d-01, &
& -1.18224255089385d-02, -7.80531239040755d-02,  8.31037990115475d-02, &
&  3.35002358178575d-01,  4.18627526940889d-01,  6.80382216561839d-02, &
& -3.66074222444200d-01,  1.45880014372833d-01,  4.44579028357735d-01, &
& -7.37898502254873d-03, -9.67361495728238d-01,  1.35276064190385d-01, &
& -2.76575099334132d-01,  1.76280352684633d-01, -1.23027713647001d-01, &
& -1.89609470467452d-01, -8.46795051995654d-02,  1.01827498108135d-01, &
&  3.87980591259518d-02, -2.94528167195311d-02,  4.84121897672957d-01, &
& -5.15513518694083d-01, -1.68239258367450d-01, -1.07672224366728d-01, &
&  1.31461873981462d+00, -4.96977474328760d+00/) 
