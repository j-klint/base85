# Base85-koodari/dekoodari


## Pikainen käyttelyohje

Samat ohjeet kuin mitkä itse ohjelma antaa komennolla
`base85 --help --ref`:

```
To encode or decode Base85/Ascii85 to stdout from a file or stdin.

Options:
  -d,   --decode  Decode instead of encoding.
  -w N, --wrap N  Insert a line break for every N characters of encoded
                  output proper. Default 75. Use 0 to disable.
  -q,   --quiet   Suppress warnings.
  -z              Disable the 'z' abbreviation for groups of zeroes.
  -y              Disable the 'y' abbreviation for groups of spaces.
        --z85     Use the Z85 alphabet. Overrides -a, forces -z -y, but
                  ignores the Z85 spec regarding input and output lengths.
        --        End of options. All arguments which come after this are
                  to be considered input file names.

  -a FILE, --alphabet FILE  Read custom alphabet from file FILE. Has to be
                            at least 85 bytes long. Bytes 86 and 87, if
                            existant, will be used for the 'z' and 'y'
                            abbreviations, respectively. If there are
                            duplicate entries, then decoding will not be
                            attempted.

  -h, --help     Display this help.
      --ref      Display references.
  -v, --version  Display version info.

If no input file is provided, then stdin is used for input. If neither custom
alphabet nor Z85 are specified, the usual one from '!' to 'u' in ASCII order
plus 'z' and 'y' will be used. During decoding all bytes not in the alphabet
will be ignored (i.e. skipped). If you want the output written to a file,
then please use the redirection operator ">" appropriately.

Wikipedia's Base85: https://en.wikipedia.org/wiki/Ascii85
The Z85 version:    https://rfc.zeromq.org/spec/32/
```

Kaiken, mikä ei ole switchiä/opšöniä, oletetaan olevan input-tiedoston nimi.
Vaikka niitä periaatteessa voikin syöttää monta, vain yksi niistä hyväksytään,
sillä mielestäni ei ole kovinkaan ilmeistä, mitä pitäisi tehdä monen tiedoston
tapauksessa.

Ja sitten tuo `--`-switch on lähinnä sitä varten, että jos joku pervo on
laittanut tiedoston nimeksi jonkin saman merkkijonon, kuin on jonkin switchin
nimi, niin eipähän tarvi sitä murehtia (toivottavasti).

Koska tietääkseni Markdownissa ei voi code blockin sisälle laittaa
hyperlinkkejä, niin tässä vielä klikattavat versiot noista kahdesta linkistä:

Wikipedia's Base85: <https://en.wikipedia.org/wiki/Ascii85><br>
The Z85 version:    <https://rfc.zeromq.org/spec/32/><br>


## Tarinointia

Kun vihdoin ryhdyin tutustumaan Linuxin/Unixin/yms.:n "ihanaan" maailmaan,
niin minuun teki vaikutuksen, kuinka mainiosti komentorivillä/terminaalissa
pystyi "piiputtamaan" yhden komennon outputin seuraavan komennon inputiksi
jne., ja kuinka, jos ohjelmoija laiskaksi ryhtyy, ohjelmalla ei edes
välttämättä tarvitse olla kykyä kirjoittaa tiedostoon, koska käyttäjä voi
aina ohjata outputin `stdout`:ista haluaamaansa tiedostoon käyttämällä
uudelleenohjausoperaattoreita `>` ja `>>`.

Sattumalta lisäksi törmäsin Juutuubissa [yli 40 vuotta vanhaan
dokumentaariin](https://www.youtube.com/watch?v=tc4ROCJYbm0), jossa esiintyy
itse Brian Kernighan ripoteltuna sinne tänne pitkin videota selittämässä
juuri näitä asioita.

Ajattelin, että minäkin haluan tehdä tuollaisen. No, tässä nyt on vähän
sellainen tekele.

On laiskasti testailtu Windows 11:ssä ja (K)ubuntu 22 LTS:ssä, ja
kummassakin vaikuttaisi toimivan.

Joo, jälkikäteen huomasin, että kyllähän `basenc` optiolla `--z85`
piiputettuna `tr`:ään ja `sed`:iin ajaisi melkein saman asian, mutta se
vaatisi vielä lisäksi ylimääräisiä pädding-kikkailuja, koska standardi-Z85
kelpuuttaa vain inputteja, joiden pituus on jaollinen 4:llä tai 5:llä,
riippuen siitä, koodataanko vai dekoodataanko.

Valitettavasti yhden kirjaimen opšönien yhdistämistä en vaivautunut
toteuttamaan, koska siinä vaiheessahan olisin jo melkein tehnyt oman version
`getopt`:ista.


## Käännösohjeita

Tämä on vielä niin pieni ohjelma, että sulloin sitten kaiken tuohon yhteen ja
ainoaan `main.cpp`-tiedostoon.

Jos on VSCoden ja C/C++-lisäkkeen lisäksi [kääntäjä asennettuna
sopivasti](https://code.visualstudio.com/docs/cpp/config-msvc)
(Windowsissa `cl.exe` ja Linuxissa `g++`) ja kaikki polut sun muut oikein,
niin pitäisi kääntyä VSCodessa pelkällä `Tasks: Run Build Task`-komennolla
(`Ctrl+Shift+B`). Tämä "Task" luo alihakemistoon `built` tiedoston `base85`
(Linuxissa) tai `base85.exe` (Windowsissa), jos kaikki menee suunnitelman
mukaan.

Jos ei onnaa, niin kuten sanottua, tässä on vain yksi tiedosto, joten
kannattaa sitä yrittää kääntää millä tahansa C++-kääntäjällä.
