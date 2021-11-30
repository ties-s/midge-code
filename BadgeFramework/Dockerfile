FROM python:3

RUN pip install --no-cache-dir pandas bluepy

COPY . /root/midge-code

WORKDIR /root/midge-code

# CMD [ "ls", "-al", "/root" ]
CMD [ "python", "hub_V1.py" ]