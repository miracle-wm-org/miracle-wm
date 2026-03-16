export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    if (url.pathname === "/" || url.pathname === "") {
      return Response.redirect(url.origin + "/miracle_plugin/", 301);
    }

    // Let Cloudflare serve the static assets normally
    return env.ASSETS.fetch(request);
  }
};
