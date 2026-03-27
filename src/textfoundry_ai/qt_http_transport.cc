#include "qt_http_transport.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include <format>

namespace tf::ai {

QtHttpTransport::QtHttpTransport(const std::chrono::milliseconds timeout)
    : timeout_(timeout) {}

Result<HttpResponse> QtHttpTransport::PostJson(
    const HttpRequest& request) const {
  if (request.url.empty()) {
    return Result<HttpResponse>(
        Error{ErrorCode::InvalidParamType, "HTTP request URL is empty"});
  }

  const QUrl url(QString::fromStdString(request.url));
  if (!url.isValid()) {
    return Result<HttpResponse>(
        Error{ErrorCode::InvalidParamType, "HTTP request URL is invalid"});
  }

  QNetworkRequest network_request(url);
  for (const auto& [key, value] : request.headers) {
    network_request.setRawHeader(QByteArray::fromStdString(key),
                                 QByteArray::fromStdString(value));
  }

  QNetworkAccessManager manager;
  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);

  QNetworkReply* reply = manager.post(
      network_request, QByteArray::fromStdString(request.body));
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  timer.start(static_cast<int>(timeout_.count()));
  loop.exec();

  if (timer.isActive()) {
    timer.stop();
  } else {
    reply->abort();
    const Error error{ErrorCode::StorageError,
                      std::format("HTTP request timed out after {} ms",
                                  timeout_.count())};
    reply->deleteLater();
    return Result<HttpResponse>(error);
  }

  if (reply->error() != QNetworkReply::NoError) {
    const Error error{
        ErrorCode::StorageError,
        "HTTP request failed: " + reply->errorString().toStdString()};
    reply->deleteLater();
    return Result<HttpResponse>(error);
  }

  const HttpResponse response{
      .status_code =
          reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
      .body = reply->readAll().toStdString(),
  };
  reply->deleteLater();
  return Result<HttpResponse>(response);
}

}  // namespace tf::ai
