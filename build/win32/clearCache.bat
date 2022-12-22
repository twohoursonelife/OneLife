@echo off
cd animations && del /s /q cache.fcz && cd ..
cd categories && del /s /q cache.fcz && cd ..
cd objects && del /s /q cache.fcz && cd ..
cd sprites && del /s /q cache.fcz && cd ..
cd transitions && del /s /q cache.fcz && cd ..
cd sprites && del /s /q *cache.fcz && cd ..
cd sounds && del /s /q *cache.fcz && cd ..
pause