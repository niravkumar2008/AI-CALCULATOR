"""Sending the link-confirmation e-mail. One small interface, two backends:

  console  logs that a mail was sent (the link itself only with MAIL_LOG_LINKS=1, for a
           bench test on your own PC; never turn that on in production)
  smtp     any SMTP provider (SMTP_HOST, SMTP_PORT, SMTP_USER, SMTP_PASSWORD, MAIL_FROM);
           STARTTLS on 587, implicit TLS on 465

Tests swap in a fake with set_mailer().
"""
import logging
import smtplib
import ssl
from email.message import EmailMessage

import config

log = logging.getLogger("calc-proxy")


class Mailer:
    def send(self, to, subject, body, link=""):
        raise NotImplementedError


class ConsoleMailer(Mailer):
    def send(self, to, subject, body, link=""):
        domain = to.split("@")[-1]
        if config.MAIL_LOG_LINKS:
            log.warning("mail (console backend) to *@%s: %s  LINK: %s", domain, subject, link)
        else:
            log.warning("mail (console backend, not delivered) to *@%s: %s. Set MAIL_BACKEND=smtp.", domain, subject)


class SmtpMailer(Mailer):
    def send(self, to, subject, body, link=""):
        msg = EmailMessage()
        msg["From"], msg["To"], msg["Subject"] = config.MAIL_FROM, to, subject
        msg.set_content(body)
        ctx = ssl.create_default_context()
        if config.SMTP_PORT == 465:
            server = smtplib.SMTP_SSL(config.SMTP_HOST, config.SMTP_PORT, context=ctx, timeout=20)
        else:
            server = smtplib.SMTP(config.SMTP_HOST, config.SMTP_PORT, timeout=20)
            server.starttls(context=ctx)
        with server:
            if config.SMTP_USER:
                server.login(config.SMTP_USER, config.SMTP_PASSWORD)
            server.send_message(msg)


_mailer = None


def get_mailer():
    global _mailer
    if _mailer is None:
        _mailer = SmtpMailer() if config.MAIL_BACKEND == "smtp" else ConsoleMailer()
    return _mailer


def set_mailer(m):
    global _mailer
    _mailer = m
