# REWRITE THIS

BP Todos before Tuesday EOD

- Document code carefully
- Write this README
- move to surge-synthesizer/clap-saw-demo

- The linux FD problem
- The RunLoop and FD management on linux is currently wrong since it doesn't deal with changing clap plugins
and doesn't deinit. 
- Let folks know

```shell
mkdir ignore
cmake -Bignore/build
cmake --build ignore/build
```
